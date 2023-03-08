import os
import json
import re
import requests

cur_dir = os.path.dirname(__file__)
contributors_path = os.path.abspath(os.path.join(cur_dir, 'contributors.json'))
changelog_path = os.path.abspath(os.path.join(cur_dir, '../../CHANGELOG.md'))

contributors = {}
raw_commits_info = {}

# dfs 建树，Merge commit 是非叶子节点
def build_commits_tree(commit_hash: str):
    res = {}
    if commit_hash not in raw_commits_info:
        return res
    raw_commit_info = raw_commits_info[commit_hash]
    if "visited" in raw_commit_info:
        return res
    author_raw_name = raw_commit_info['author']
    author_name = author_raw_name
    if author_raw_name not in contributors:
        try:
            github_info = requests.get(f"https://api.github.com/repos/MaaAssistantArknights/MaaAssistantArknights/commits/{commit_hash}")
            author_name = contributors[author_raw_name] = json.loads(github_info.text)['author']['login']
            print(author_raw_name, ":", author_name)
        except:
            print(f"Cannot get author: {author_raw_name}.")
    raw_commit_info.update({"visited": True})
    commit_key = raw_commit_info["message"] + f" ({raw_commit_info['hash']})" + f" @{author_name}"
    res[commit_key] = {}
    res.update(build_commits_tree(raw_commit_info["parent"][0]))
    if len(raw_commit_info["parent"]) == 2:
        if commit_key.startswith("Release"):
            # 避免合并之后只有一个 Release 主 commit
            res.update(build_commits_tree(raw_commit_info["parent"][1]))
        else:
            res[commit_key].update(build_commits_tree(raw_commit_info["parent"][1]))
        if commit_key.startswith("Merge branch") and not res[commit_key]:
            res.pop(commit_key)
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
        for x in commits.keys():
            if False:
                pass
            elif x.find("新增") != -1:
                sorted_commits["feat"][x] = commits[x]
            elif x.find("改进") != -1 or x.find("更新") != -1 or x.find("优化") != -1 or x.find("重构") != -1:
                sorted_commits["perf"][x] = commits[x]
            elif x.find("修复") != -1:
                sorted_commits["fix"][x] = commits[x]
            elif x.startswith("feat"):
                sorted_commits["feat"][x] = commits[x]
            elif x.startswith("perf"):
                sorted_commits["perf"][x] = commits[x]
            elif x.startswith("fix"):
                sorted_commits["fix"][x] = commits[x]
            else:
                sorted_commits["other"][x] = commits[x]

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
        for x in reversed(sorted([x for x in commits.keys()])):
            commit_message = x
            # Contributors 名字转换
            contributor_re = re.search(r"@\S*", x)
            if not contributor_re:
                print(f"Warning: `{x}` has no contributor!")
            else:
                contributor = contributor_re.group()
                commit_message = re.sub(r"@\S*", "", commit_message)

            mes, ctrs = print_commits(commits[x], indent + "   ", False)
            ret_message += indent + "- " + commit_message

            if ctrs.count(contributor) == 0:
                ret_message += contributor
                if ret_contributor.count(contributor) == 0:
                    ret_contributor.append(contributor)
            for ctr in ctrs:
                if ret_contributor.count(ctr) == 0:
                    ret_contributor.append(ctr)
                ret_message += " " + ctr
            ret_message += "\n" + mes

    return ret_message, ret_contributor

def main():
    global contributors, raw_commits_info
    try:
        with open(contributors_path, "r") as f:
            contributors = json.load(f)
    except:
        pass
    # 从哪个 tag 开始
    latest = os.popen("git describe --abbrev=0 --tags").read()[:-1]
    nightly = os.popen("git describe --tags").read()[:-1]
    print("From:", latest, ", To:", nightly, "\n")

    # 输出一张好看的 git log 图到控制台
    git_pretty_command = rf'git log {latest}..HEAD --pretty=format:"%C(yellow)%d%Creset %s %C(bold blue)@%an%Creset (%Cgreen%h%Creset)" --graph'
    os.system(git_pretty_command)
    print()

    # 获取详细的 git log 信息
    git_command = rf'git log {latest}..HEAD --pretty=format:"\"%H\":{{\"hash\":\"%h\",\"author\":\"%aN\",\"committer\":\"%cN\",\"message\":\"%s\",\"parent\":\"%P\"}},"'

    with os.popen(git_command) as fp: bf = fp._stream.buffer.read()
    try: gitlogs = bf.decode().strip()
    except: gitlogs = bf.decode("gbk").strip()
    gitlogs = "{" + gitlogs[:-1] + "}"
    raw_commits_info = json.loads(gitlogs)
    for x, y in raw_commits_info.items():
        y["parent"] = y["parent"].split()
    # print(json.dumps(raw_commits_info, ensure_ascii=False, indent=2))

    res = print_commits(build_commits_tree([x for x in raw_commits_info.keys()][0]))

    with open(changelog_path, "w", encoding="utf8") as f:
        f.write("## " + nightly + "\n" + res[0])

    with open(contributors_path, "w") as f:
        json.dump(contributors, f)

if __name__ == "__main__":
    main()