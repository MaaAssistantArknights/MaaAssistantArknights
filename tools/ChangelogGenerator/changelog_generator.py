from argparse import ArgumentParser
from enum import Enum
from typing import Tuple
from pathlib import Path
import os
import json
import re
import urllib.request
import urllib.error

repo = "MaaAssistantArknights/MaaAssistantArknights" # Owner/RepoName
cur_dir = Path(__file__).parent
contributors_path = cur_dir / "contributors.json"
changelog_path = cur_dir.parent.parent / "CHANGELOG.md"

with_hash = False
with_commitizen = False
committer_is_author = False
merge_author = False
commitizens = r"(?:build|chore|ci|docs?|feat!?|fix|perf|refactor|rft|style|test|i18n|typo|debug)"
ignore_commitizens = r"(?:build|ci|style|debug)"

contributors = {}
raw_commits_info = {}


class Translation(Enum):
    FIX = "fix"
    FEAT = "feat"
    PERF = "perf"
    DOCS = "docs"
    OTHER = "other"


translations = {
    "修复": Translation.FIX,
    "新增": Translation.FEAT,
    "更新": Translation.PERF,
    "改进": Translation.PERF,
    "优化": Translation.PERF,
    "重构": Translation.PERF,
    "文档": Translation.DOCS,
    "其他": Translation.OTHER,
}

translations_resort = {
    "新增 | New": Translation.FEAT,
    "改进 | Improved": Translation.PERF,
    "修复 | Fix": Translation.FIX,
    "文档 | Docs": Translation.DOCS,
    "其他 | Other": Translation.OTHER,
}


def individual_commits(commits: dict, indent: str = "") -> Tuple[str, list]:
    if not commits:
        return ("", [])
    ret_message = ""
    ret_contributor = []

    for commit_hash, commit_info in commits.items():
        if commit_info["skip"]:
            continue

        commit_message = commit_info["message"]

        if re.match(rf"^{ignore_commitizens} *(?:\([^\)]*\))*: *", commit_message):
            continue

        if not with_commitizen:
            commit_message = re.sub(
                rf"^(?:{commitizens}, *)*{commitizens} *(?:\([^\)]*\))*: *",
                "",
                commit_message,
            )

        ret_message += indent + "* " + commit_message

        mes, ctrs = individual_commits(commit_info["branch"], indent + "   ")

        if merge_author or not commit_info["branch"]:
            ctrs.extend(
                ctr
                for ctr in [
                    commit_info["author"],
                    *commit_info.get("coauthors", []),
                    commit_info["committer"] if committer_is_author else None,
                ]
                if ctr not in ctrs and ctr is not None
            )

        ret_contributor += [
            ctr for ctr in ctrs if ctr != "web-flow" and ctr not in ret_contributor
        ]
        ret_message += "".join(f" @{ctr}" for ctr in ctrs if ctr != "web-flow")

        ret_message += f" ({commit_hash})\n" if with_hash else "\n"
        if with_merge:
            ret_message += mes

    return ret_message, ret_contributor


def update_commits(commit_message, sorted_commits, update_dict):
    oper = "other"
    # 优先检查 commit_message 中是否有明确的前缀
    for trans in Translation:
        if commit_message.startswith(trans.value):
            oper = trans.value
            break
    else:
        # 如果没有明确前缀，则检查翻译的中文关键词
        for key, trans in translations.items():
            if key in commit_message:
                oper = trans.value
                break

    sorted_commits[oper].update(update_dict)


def update_message(sorted_commits, ret_contributor):
    ret_message = ""
    for key, trans in translations_resort.items():
        if sorted_commits[trans.value]:
            mes, ctrs = individual_commits(sorted_commits[trans.value], "")
            if mes:
                ret_message += f"\n### {key}\n\n"
                ret_message += mes
            for ctr in ctrs:
                if ret_contributor.count(ctr) == 0:
                    ret_contributor.append(ctr)
    return (ret_message,)


def print_commits(commits: dict):
    sorted_commits = {
        "perf": {},
        "feat": {},
        "fix": {},
        "docs": {},
        "other": {},
    }
    for commit_hash, commit_info in commits.items():
        commit_message = commit_info["message"]
        update_commits(commit_message, sorted_commits, {commit_hash: commit_info})

    return update_message(sorted_commits, [])


def build_commits_tree(commit_hash: str):
    """
    返回值为当前 commit 与其 parents 的信息。
    返回值结构：
    {
        commit_hash: {
            "hash": str,
            "author": str,
            "committer": str,
            "coauthors": [str, ...],
            "message": str,
            "branch": {} # 当当前为 merge commit 时非空，为被合并分支对应的 commits 信息
        },
        ...
    }
    """
    if commit_hash not in raw_commits_info:
        return {}
    raw_commit_info = raw_commits_info[commit_hash]
    if "visited" in raw_commit_info and raw_commit_info["visited"]:
        return {}
    raw_commit_info.update({"visited": True})  # 防止一个 commit 被多个分支遍历

    res = {
        commit_hash: {
            "hash": raw_commit_info["hash"],
            "author": raw_commit_info["author"],
            "committer": raw_commit_info["committer"],
            "coauthors": raw_commit_info.get("coauthors", []),
            "message": raw_commit_info["message"],
            "branch": {},
            "skip": raw_commit_info.get("skip", False),
        }
    }

    res.update(build_commits_tree(raw_commit_info["parent"][0]))

    # 第二个 parent 为 Merge commit 的被合并分支
    if len(raw_commit_info["parent"]) == 2:
        if raw_commit_info["message"].startswith("Release") or raw_commit_info[
            "message"
        ].startswith("Merge"):
            # 避免合并之后只有一个 Release 主 commit
            # 忽略不带信息的 Merge commit (eg. Merge remote-tracking branch; Merge branch 'dev' of xxx into dev)
            res.update(build_commits_tree(raw_commit_info["parent"][1]))
        else:
            res[commit_hash]["branch"].update(
                build_commits_tree(raw_commit_info["parent"][1])
            )
        if (
            raw_commit_info["message"].startswith("Merge")
            and not res[commit_hash]["branch"]
        ):
            res.pop(commit_hash)
    return res


def retry_urlopen(*args, **kwargs):
    import time
    import http.client

    for _ in range(5):
        try:
            resp: http.client.HTTPResponse = urllib.request.urlopen(*args, **kwargs)
            return resp
        except urllib.error.HTTPError as e:
            if e.status == 403 and e.headers.get("x-ratelimit-remaining") == "0":
                # rate limit
                t0 = time.time()
                reset_time = t0 + 10
                try:
                    reset_time = int(e.headers.get("x-ratelimit-reset", 0))
                except ValueError:
                    pass
                reset_time = max(reset_time, t0 + 10)
                print(
                    f"rate limit exceeded, retrying after {reset_time - t0:.1f} seconds"
                )
                time.sleep(reset_time - t0)
                continue
            raise


# 贡献者名字转账号名
def convert_contributors_name(name: str, commit_hash: str, name_type: str):
    global contributors
    if name not in contributors:
        try:
            req = urllib.request.Request(
                f"https://api.github.com/repos/{repo}/commits/{commit_hash}"
            )
            token = os.environ.get("GH_TOKEN", os.environ.get("GITHUB_TOKEN", None))
            if token:
                req.add_header("Authorization", f"Bearer {token}")
            resp = retry_urlopen(req).read()
            userid = json.loads(resp)[name_type]["login"]
            contributors.update({name: userid})
            return userid
        except Exception as e:
            print(f"Cannot get {name_type}: {name}. ({e})")
            return name
    else:
        return contributors[name]


def call_command(command: str):
    with os.popen(command) as fp:
        bf = fp._stream.buffer.read()
    try:
        command_ret = bf.decode().strip()
    except:
        command_ret = bf.decode("gbk").strip()
    return command_ret


def main(tag_name=None, latest=None):
    global contributors, raw_commits_info
    try:
        with open(contributors_path, "r") as f:
            contributors = json.load(f)
    except:
        pass
    # 从哪个 tag 开始
    if not latest:
        latest = os.popen('git describe --tags --match "v*" --abbrev=0').read().strip()

    if not tag_name:
        tag_name = os.popen('git describe --tags --match "v*"').read().strip()
    print("From:", latest, ", To:", tag_name, "\n")

    # 输出一张好看的 git log 图到控制台
    # git_pretty_command = rf'git log {latest}..HEAD --pretty=format:"%C(yellow)%d%Creset %s %C(bold blue)@%an%Creset (%Cgreen%h%Creset)" --graph'
    # os.system(git_pretty_command)

    # 获取详细的 git log 信息
    # git_command = rf'git log {latest}..HEAD --pretty=format:"\"%H\":{{\"hash\":\"%h\",\"author\":\"%aN\",\"committer\":\"%cN\",\"message\":\"%s\",\"parent\":\"%P\"}},"'
    git_command = rf'git log {latest}..HEAD --pretty=format:"%H%n%aN%n%cN%n%s%n%P%n"'
    raw_gitlogs = call_command(git_command)

    raw_commits_info = {}
    # In case the check step fails in the workflow, prevent exit error 1
    if raw_gitlogs.strip():
        for raw_commit_info in raw_gitlogs.split("\n\n"):
            commit_hash, author, committer, message, parent = raw_commit_info.split("\n")

            author = convert_contributors_name(
                name=author, commit_hash=commit_hash, name_type="author"
            )
            committer = convert_contributors_name(
                name=committer, commit_hash=commit_hash, name_type="committer"
            )

            raw_commits_info[commit_hash] = {
                "hash": commit_hash[:8],
                "author": author,
                "committer": committer,
                "message": message,
                "parent": parent.split(),
            }

        git_coauthor_command = (
            rf'git log {latest}..HEAD --pretty=format:"%H%n" --grep="Co-authored-by"'
        )
        raw_gitlogs = call_command(git_coauthor_command)

        for commit_hash in raw_gitlogs.split("\n"):
            if commit_hash not in raw_commits_info:
                continue
            git_addition_command = rf'git log {commit_hash} --no-walk --pretty=format:"%b"'
            addition = call_command(git_addition_command)
            coauthors = []
            for coauthor in re.findall(r"Co-authored-by: (.*) <(?:.*)>", addition):
                if coauthor in contributors:
                    coauthors.append(contributors[coauthor])
                elif coauthor in contributors.values():
                    coauthors.append(coauthor)
                else:
                    print(f"Cannot get coauthor: {coauthor}.")
            raw_commits_info[commit_hash]["coauthors"] = coauthors

        git_skip_command = (
            rf'git log {latest}..HEAD --pretty=format:"%H%n" --grep="\[skip changelog\]"'
        )
        raw_gitlogs = call_command(git_skip_command)

        for commit_hash in raw_gitlogs.split("\n\n"):
            if commit_hash not in raw_commits_info:
                continue
            git_show_command = (
                rf'git show -s --format=%B%n {commit_hash}'
            )
            raw_git_shows = call_command(git_show_command)
            for commit_body in raw_git_shows.split("\n"):
                if not commit_body.startswith("* ") and "[skip changelog]" in commit_body:
                    raw_commits_info[commit_hash]["skip"] = True


        # print(json.dumps(raw_commits_info, ensure_ascii=False, indent=2))

        res = print_commits(build_commits_tree([x for x in raw_commits_info.keys()][0]))

        changelog_content = "## " + tag_name + "\n" + res[0]
        print(changelog_content)
        with open(changelog_path, "w", encoding="utf8") as f:
            f.write(changelog_content)

        with open(contributors_path, "w") as f:
            json.dump(contributors, f)

    # In case the check step fails in the workflow, prevent exit error 1
    else:
        print("No commits found.")
        with open(os.getenv('GITHUB_OUTPUT'), 'a') as github_output: github_output.write("cancel_run=true\n")

def ArgParser():
    parser = ArgumentParser()
    parser.add_argument(
        "--tag", help="release tag name", metavar="TAG", dest="tag_name", default=None
    )
    parser.add_argument(
        "--base",
        "--latest",
        help="base tag name",
        metavar="TAG",
        dest="latest",
        default=None,
    )
    parser.add_argument(
        "-wh",
        "--with-hash",
        help="print commit message with hash",
        action="store_true",
        dest="with_hash",
    )
    parser.add_argument(
        "-wc",
        "--with-commitizen",
        help="print commit message with commitizen",
        action="store_true",
        dest="with_commitizen",
    )
    parser.add_argument(
        "-ma",
        "--merge-author",
        help="do not ignore merge author",
        action="store_true",
        dest="merge_author",
    )
    parser.add_argument(
        "-ca",
        "--committer-is-author",
        help="treat committer the same as author",
        action="store_true",
        dest="committer_is_author",
    )
    parser.add_argument(
        "-wm",
        "--with-merge",
        help="print merge commits tree",
        action="store_true",
        dest="with_merge",
    )
    return parser


if __name__ == "__main__":
    args = ArgParser().parse_args()
    with_hash = args.with_hash
    with_commitizen = args.with_commitizen
    with_merge = args.with_merge
    latest = args.latest
    tag_name = args.tag_name
    merge_author = args.merge_author
    committer_is_author = args.committer_is_author
    main(tag_name=tag_name, latest=latest)
