from argparse import ArgumentParser
import os
import json
import re
import urllib.request
import urllib.error

cur_dir = os.path.dirname(__file__)
contributors_path = os.path.abspath(os.path.join(cur_dir, 'contributors.json'))
changelog_path = os.path.abspath(os.path.join(cur_dir, '../../CHANGELOG.md'))

with_hash = False
with_commitizen = False
committer_is_author = False
ignore_merge_author = False

contributors = {}
raw_commits_info = {}

def build_commits_tree(commit_hash: str):
    '''
    返回值为当前 commit 与其 parents 的信息。
    返回值结构：
    {
        commit_hash: {
            "hash": str,
            "author": str,
            "committer": str,
            "message": str,
            "branch": {} # 当当前为 merge commit 时非空，为被合并分支对应的 commits 信息
        },
        ...
    }
    '''
    if commit_hash not in raw_commits_info:
        return {}
    raw_commit_info = raw_commits_info[commit_hash]
    if "visited" in raw_commit_info and raw_commit_info["visited"]:
        return {}
    raw_commit_info.update({"visited": True}) # 防止一个 commit 被多个分支遍历

    commit_hash = raw_commit_info["hash"]
    res = {
        commit_hash: {
            "hash": commit_hash,
            "author": raw_commit_info["author"],
            "committer": raw_commit_info["committer"],
            "message": raw_commit_info["message"],
            "branch": {}
        }
    }

    res.update(build_commits_tree(raw_commit_info["parent"][0]))

    # 第二个 parent 为 Merge commit 的被合并分支
    if len(raw_commit_info["parent"]) == 2:
        if (raw_commit_info["message"].startswith("Release") or
            re.match(r"Merge pull request #\d+ from MaaAssistantArknights/dev", raw_commit_info["message"])):
            # 避免合并之后只有一个 Release 主 commit
            # 忽略从 dev 合并的 Merge commit
            res.update(build_commits_tree(raw_commit_info["parent"][1]))
        else:
            res[commit_hash]["branch"].update(build_commits_tree(raw_commit_info["parent"][1]))
        if raw_commit_info["message"].startswith("Merge branch") and not res[commit_hash]["branch"]:
            res.pop(commit_hash)
    return res

def print_commits(commits: dict, indent: str = "", need_sort: bool = True) -> (str, list):
    if not commits: return ("", [])
    ret_message = ""
    ret_contributor = []

    sorted_commits = {
        "perf": {},
        "feat": {},
        "fix": {},
        "other": {},
    }
    if need_sort and indent == "":
        for commit_hash, commit_info in commits.items():
            commit_message = commit_info["message"]
            if False:
                pass
            elif commit_message.find("修复") != -1:
                sorted_commits["fix"].update({commit_hash: commit_info})
            elif commit_message.find("新增") != -1:
                sorted_commits["feat"].update({commit_hash: commit_info})
            elif commit_message.find("改进") != -1 or commit_message.find("更新") != -1 or commit_message.find("优化") != -1 or commit_message.find("重构") != -1:
                sorted_commits["perf"].update({commit_hash: commit_info})
            elif commit_message.startswith("feat"):
                sorted_commits["feat"].update({commit_hash: commit_info})
            elif commit_message.startswith("perf"):
                sorted_commits["perf"].update({commit_hash: commit_info})
            elif commit_message.startswith("fix"):
                sorted_commits["fix"].update({commit_hash: commit_info})
            else:
                sorted_commits["other"].update({commit_hash: commit_info})

        if sorted_commits["feat"]:
            ret_message += "\n### 新增\n\n"
            mes, ctrs = print_commits(sorted_commits["feat"], "", False)
            ret_message += mes
            for ctr in ctrs:
                if ret_contributor.count(ctr) == 0:
                    ret_contributor.append(ctr)

        if sorted_commits["perf"]:
            ret_message += "\n### 改进\n\n"
            mes, ctrs = print_commits(sorted_commits["perf"], "", False)
            ret_message += mes
            for ctr in ctrs:
                if ret_contributor.count(ctr) == 0:
                    ret_contributor.append(ctr)
        if sorted_commits["fix"]:
            ret_message += "\n### 修复\n\n"
            mes, ctrs = print_commits(sorted_commits["fix"], "", False)
            ret_message += mes
            for ctr in ctrs:
                if ret_contributor.count(ctr) == 0:
                    ret_contributor.append(ctr)
        if sorted_commits["other"]:
            ret_message += "\n### 其他\n\n"
            mes, ctrs = print_commits(sorted_commits["other"], "", False)
            ret_message += mes
            for ctr in ctrs:
                if ret_contributor.count(ctr) == 0:
                    ret_contributor.append(ctr)

    else:
        for commit_hash, commit_info in commits.items():
            commit_message = commit_info["message"]

            if not with_commitizen:
                commitizens = r"(?:build|chore|ci|docs?|feat|fix|perf|refactor|rft|style|test)"
                commit_message = re.sub(rf"^(?:{commitizens}, *)*{commitizens} *(?:\([^\)]*\))*: *", "", commit_message)

            ret_message += indent + "- " + commit_message

            mes, ctrs = print_commits(commit_info["branch"], indent + "   ", False)

            if not ignore_merge_author or not commit_info["branch"]:
                author = commit_info["author"]
                if author not in ctrs: ctrs.append(author)
                if committer_is_author:
                    committer = commit_info["committer"]
                    if committer not in ctrs: ctrs.append(committer)

            for ctr in ctrs:
                if ctr == "web-flow": continue # 这个账号是 GitHub 在 Merge PR 时的 committer
                if ret_contributor.count(ctr) == 0:
                    ret_contributor.append(ctr)
                ret_message += " @" + ctr

            if with_hash:
                ret_message += f" ({commit_hash})"

            ret_message += "\n" + mes

    return ret_message, ret_contributor


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
                print(f"rate limit exceeded, retrying after {reset_time - t0:.1f} seconds")
                time.sleep(reset_time - t0)
                continue
            raise


# 贡献者名字转账号名
def convert_contributors_name(name: str, commit_hash: str, name_type: str):
    global contributors
    if name not in contributors:
        try:
            req = urllib.request.Request(f"https://api.github.com/repos/MaaAssistantArknights/MaaAssistantArknights/commits/{commit_hash}")
            token = os.environ.get("GH_TOKEN", os.environ.get("GITHUB_TOKEN", None))
            if token:
                req.add_header("Authorization", f"Bearer {token}")
            resp = retry_urlopen(req).read()
            userid = json.loads(resp)[name_type]['login']
            contributors.update({name: userid})
            return userid
        except Exception as e:
            print(f"Cannot get {name_type}: {name}. ({e})")
            return name
    else:
        return contributors[name]

def main(tag_name=None, latest=None):
    global contributors, raw_commits_info
    try:
        with open(contributors_path, "r") as f:
            contributors = json.load(f)
    except:
        pass
    # 从哪个 tag 开始
    if not latest:
        latest = os.popen("git describe --tags --match \"v*\" --abbrev=0").read().strip()

    if not tag_name:
        tag_name = os.popen("git describe --tags --match \"v*\"").read().strip()
    print("From:", latest, ", To:", tag_name, "\n")

    # 输出一张好看的 git log 图到控制台
    git_pretty_command = rf'git log {latest}..HEAD --pretty=format:"%C(yellow)%d%Creset %s %C(bold blue)@%an%Creset (%Cgreen%h%Creset)" --graph'
    # os.system(git_pretty_command)

    # 获取详细的 git log 信息
    # git_command = rf'git log {latest}..HEAD --pretty=format:"\"%H\":{{\"hash\":\"%h\",\"author\":\"%aN\",\"committer\":\"%cN\",\"message\":\"%s\",\"parent\":\"%P\"}},"'
    git_command = rf'git log {latest}..HEAD --pretty=format:"%H%n%aN%n%cN%n%s%n%P%n"'

    with os.popen(git_command) as fp: bf = fp._stream.buffer.read()
    try: raw_gitlogs = bf.decode().strip()
    except: raw_gitlogs = bf.decode("gbk").strip()

    raw_commits_info = {}
    for raw_commit_info in raw_gitlogs.split('\n\n'):
        commit_hash, author, committer, message, parent = raw_commit_info.split('\n')

        author = convert_contributors_name(name=author, commit_hash=commit_hash, name_type="author")
        committer = convert_contributors_name(name=committer, commit_hash=commit_hash, name_type="committer")

        raw_commits_info[commit_hash] = {
            "hash": commit_hash[:8],
            "author": author,
            "committer": committer,
            "message": message,
            "parent": parent.split()
        }
    # print(json.dumps(raw_commits_info, ensure_ascii=False, indent=2))

    res = print_commits(build_commits_tree([x for x in raw_commits_info.keys()][0]))

    changelog_content = "## " + tag_name + "\n" + res[0]
    print(changelog_content)
    with open(changelog_path, "w", encoding="utf8") as f:
        f.write(changelog_content)

    with open(contributors_path, "w") as f:
        json.dump(contributors, f)

def ArgParser():
    parser = ArgumentParser()
    parser.add_argument("--tag", help="release tag name", metavar="TAG", dest="tag_name", default=None)
    parser.add_argument("--base", "--latest", help="base tag name", metavar="TAG", dest="latest", default=None)
    parser.add_argument("-wh", "--with-hash", help="print commit message with hash", action="store_true", dest="with_hash")
    parser.add_argument("-wc", "--with-commitizen", help="print commit message with commitizen", action="store_true", dest="with_commitizen")
    parser.add_argument("-im", "--ignore-merge-author", help="ignore merge author", action="store_true", dest="ignore_merge_author", default=True)
    parser.add_argument("-ca", "--committer-is-author", help="treat committer the same as author", action="store_true", dest="committer_is_author")
    return parser

if __name__ == "__main__":
    args = ArgParser().parse_args()
    with_hash = args.with_hash
    with_commitizen = args.with_commitizen
    latest = args.latest
    tag_name = args.tag_name
    ignore_merge_author = args.ignore_merge_author
    committer_is_author = args.committer_is_author
    main(tag_name=tag_name, latest=latest)
