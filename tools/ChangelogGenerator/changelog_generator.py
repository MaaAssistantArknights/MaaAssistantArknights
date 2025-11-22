import json
import os
import re
import urllib.error
import urllib.request
from argparse import ArgumentParser
from enum import Enum
from pathlib import Path
from typing import Tuple

repo = "MaaAssistantArknights/MaaAssistantArknights"  # Owner/RepoName
cur_dir = Path(__file__).parent
contributors_path = cur_dir / "contributors.json"
changelog_path = cur_dir.parent.parent / "CHANGELOG.md"

with_hash = False
with_commitizen = False
committer_is_author = False
merge_author = False
with_merge = False

contributors = {}
raw_commits_info = {}

IGNORE_PREFIXES = r"(?:build|ci|style|debug)"

# 中文关键词映射到分类
translations = {
    "修复": "fix",
    "新增": "feat",
    "更新": "perf",
    "改进": "perf",
    "优化": "perf",
    "重构": "perf",
    "文档": "docs",
    "其他": "other",
}

translations_resort = {
    "新增 | New": "feat",
    "改进 | Improved": "perf",
    "修复 | Fix": "fix",
    "文档 | Docs": "docs",
    "其他 | Other": "other",
}


def parse_category(message: str) -> str:
    """
    根据 commitizen 前缀或中文关键字返回分类
    """
    # 1. 检查 commitizen 前缀

    if re.match(rf"^{IGNORE_PREFIXES} *(?:\([^\)]*\))*: *", message):
        return None  # 返回 None 表示跳过

    m = re.match(r"^(?P<prefix>\w+)(?:\([\w\-]+\))?:\s*", message)
    if m:
        prefix = m.group("prefix").lower()
        if prefix == "feat":
            return "feat"
        if prefix == "fix":
            return "fix"
        if prefix in ["perf", "refactor", "rft"]:
            return "perf"
        if prefix in ["docs", "doc"]:
            return "docs"
        return "other"

    # 2. 中文关键词匹配
    for key, cat in translations.items():
        if key in message:
            return cat

    return "other"


def individual_commits(commits: dict, indent: str = "") -> Tuple[str, list]:
    if not commits:
        return "", []

    ret_message = ""
    ret_contributor = []

    for commit_hash, commit_info in commits.items():
        if commit_info.get("skip"):
            continue

        commit_message = commit_info["message"]

        # 剥掉 commitizen 前缀，除非保留
        if not with_commitizen:
            commit_message = re.sub(
                r"^(?P<prefix>\w+)(?:\([\w\-]+\))?:\s*", "", commit_message
            )

        # 递归处理 merge branch
        mes, ctrs = individual_commits(commit_info.get("branch", {}), indent + "   ")

        # 收集作者
        if merge_author or not commit_info.get("branch"):
            all_authors = [
                commit_info.get("author"),
                *commit_info.get("coauthors", []),
                commit_info.get("committer") if committer_is_author else None,
            ]
            ctrs.extend([c for c in all_authors if c and c not in ctrs])

        ret_contributor.extend(
            [c for c in ctrs if c != "web-flow" and c not in ret_contributor]
        )

        # 拼接 commit message
        ret_message += indent + "* " + commit_message
        ret_message += "".join(f" @{ctr}" for ctr in ctrs if ctr != "web-flow")
        ret_message += f" ({commit_hash})\n" if with_hash else "\n"

        if with_merge:
            ret_message += mes

    return ret_message, ret_contributor


def update_commits(commit_message, sorted_commits, update_dict):
    category = parse_category(commit_message)
    if not category:  # 如果返回 None，跳过
        return
    sorted_commits[category].update(update_dict)


def update_message(sorted_commits, ret_contributor):
    ret_message = ""
    for key, category in translations_resort.items():
        if sorted_commits[category]:
            mes, ctrs = individual_commits(sorted_commits[category], "")
            if mes:
                ret_message += f"\n### {key}\n\n{mes}"
            for ctr in ctrs:
                if ctr not in ret_contributor:
                    ret_contributor.append(ctr)
    return (ret_message,)


def print_commits(commits: dict):
    sorted_commits = {cat: {} for cat in ["perf", "feat", "fix", "docs", "other"]}
    for commit_hash, commit_info in commits.items():
        update_commits(
            commit_info["message"], sorted_commits, {commit_hash: commit_info}
        )
    return update_message(sorted_commits, [])


def build_commits_tree(commit_hash: str):
    if commit_hash not in raw_commits_info:
        return {}
    commit_info = raw_commits_info[commit_hash]
    if commit_info.get("visited"):
        return {}
    commit_info["visited"] = True

    res = {
        commit_hash: {
            "hash": commit_info["hash"],
            "author": commit_info["author"],
            "committer": commit_info["committer"],
            "coauthors": commit_info.get("coauthors", []),
            "message": commit_info["message"],
            "branch": {},
            "skip": commit_info.get("skip", False),
        }
    }

    # 递归父 commit
    res |= build_commits_tree(commit_info["parent"][0])

    if len(commit_info["parent"]) == 2:
        # merge 分支处理
        if commit_info["message"].startswith(("Release", "Merge")):
            res.update(build_commits_tree(commit_info["parent"][1]))
        else:
            res[commit_hash]["branch"].update(
                build_commits_tree(commit_info["parent"][1])
            )
        if (
            commit_info["message"].startswith("Merge")
            and not res[commit_hash]["branch"]
        ):
            res.pop(commit_hash)

    return res


def retry_urlopen(*args, **kwargs):
    import http.client
    import time

    for _ in range(5):
        try:
            return urllib.request.urlopen(*args, **kwargs)
        except urllib.error.HTTPError as e:
            if e.status == 403 and e.headers.get("x-ratelimit-remaining") == "0":
                t0 = time.time()
                try:
                    reset_time = int(e.headers.get("x-ratelimit-reset", 0))
                except ValueError:
                    reset_time = t0 + 10
                time.sleep(max(reset_time - t0, 10))
                continue
            raise


def convert_contributors_name(name: str, commit_hash: str, name_type: str):
    global contributors
    if name in contributors:
        return contributors[name]
    try:
        req = urllib.request.Request(
            f"https://api.github.com/repos/{repo}/commits/{commit_hash}"
        )
        token = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")
        if token:
            req.add_header("Authorization", f"Bearer {token}")
        resp = retry_urlopen(req).read()
        userid = json.loads(resp)[name_type]["login"]
        contributors[name] = userid
        return userid
    except Exception as e:
        print(f"Cannot get {name_type}: {name}. ({e})")
        return name


def call_command(command: str):
    with os.popen(command) as fp:
        bf = fp._stream.buffer.read()
    try:
        return bf.decode().strip()
    except:
        return bf.decode("gbk").strip()


def main(tag_name=None, latest=None):
    global contributors, raw_commits_info

    try:
        with open(contributors_path, "r") as f:
            contributors = json.load(f)
    except:
        contributors = {}

    if not latest:
        latest = call_command('git describe --tags --match "v*" --abbrev=0')
    if not tag_name:
        tag_name = call_command('git describe --tags --match "v*"')

    print("From:", latest, ", To:", tag_name, "\n")

    git_command = rf'git log {latest}..HEAD --pretty=format:"%H%n%aN%n%cN%n%s%n%P%n"'
    raw_gitlogs = call_command(git_command)

    raw_commits_info = {}
    if raw_gitlogs.strip():
        for raw_commit_info in raw_gitlogs.split("\n\n"):
            commit_hash, author, committer, message, parent = raw_commit_info.split(
                "\n"
            )
            author = convert_contributors_name(author, commit_hash, "author")
            committer = convert_contributors_name(committer, commit_hash, "committer")
            raw_commits_info[commit_hash] = {
                "hash": commit_hash[:8],
                "author": author,
                "committer": committer,
                "message": message,
                "parent": parent.split(),
            }

        # coauthor
        git_coauthor_command = (
            rf'git log {latest}..HEAD --pretty=format:"%H%n" --grep="Co-authored-by"'
        )
        coauthor_hashes = call_command(git_coauthor_command).split("\n")
        for commit_hash in coauthor_hashes:
            if commit_hash not in raw_commits_info:
                continue
            addition = call_command(
                rf'git log {commit_hash} --no-walk --pretty=format:"%b"'
            )
            coauthors = []
            for coauthor in re.findall(r"Co-authored-by: (.*) <(?:.*)>", addition):
                if coauthor in contributors:
                    coauthors.append(contributors[coauthor])
                elif coauthor in contributors.values():
                    coauthors.append(coauthor)
                else:
                    print(f"Cannot get coauthor: {coauthor}.")
            raw_commits_info[commit_hash]["coauthors"] = coauthors

        # skip changelog
        git_skip_command = rf'git log {latest}..HEAD --pretty=format:"%H%n" --grep="\[skip changelog\]"'
        skip_hashes = call_command(git_skip_command).split("\n")
        for commit_hash in skip_hashes:
            if commit_hash not in raw_commits_info:
                continue
            raw_git_show = call_command(f"git show -s --format=%B%n {commit_hash}")
            if "[skip changelog]" in raw_git_show:
                raw_commits_info[commit_hash]["skip"] = True

        # build changelog
        first_hash = list(raw_commits_info.keys())[0]
        res = print_commits(build_commits_tree(first_hash))
        changelog_content = "## " + tag_name + "\n" + res[0]
        print(changelog_content)
        with open(changelog_path, "w", encoding="utf8") as f:
            f.write(changelog_content)

        with open(contributors_path, "w") as f:
            json.dump(contributors, f)
    else:
        print("No commits found.")
        github_output = os.getenv("GITHUB_OUTPUT")
        if github_output:
            with open(github_output, "a") as f:
                f.write("cancel_run=true\n")


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
