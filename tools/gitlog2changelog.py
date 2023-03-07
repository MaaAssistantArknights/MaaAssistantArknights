import os
import json
import re

contributors = {
    "@[Yifan Liu]": "@liuyifan-eric",
    "@[ABA2396]": "@ABA2396",
    "@[uye]": "@ABA2396",
    "@[Horror Proton]": "@horror-proton",
    "@[zzyyyl]": "@zzyyyl",
    "@[MistEO]": "@MistEO",
    "@[dantmnf]": "@dantmnf",
    "@[Hao Guan]": "@hguandl",
}

# 从哪个 tag 开始
latest = os.popen("git describe --abbrev=0 --tags").read()[:-1]
print("From:", latest, "\n")

# 输出一张好看的 git log 图到控制台
git_pretty_command = rf'git log {latest}..HEAD --pretty=format:"%C(yellow)%d%Creset %s %C(bold blue)@%an%Creset (%Cgreen%h%Creset)" --graph'
os.system(git_pretty_command)

# 获取详细的 git log 信息
git_command = rf'git log {latest}..HEAD --pretty=raw'
with os.popen(git_command) as fp: bf = fp._stream.buffer.read()
try: gitlogs = bf.decode().strip()
except: gitlogs = bf.decode("gbk").strip()

current_commit = None
current_commit_status = ""
raw_commits_info = {}

for line in gitlogs.split("\n"):
    line = re.sub(r"^ *", "", line)
    if line.startswith("commit "):
        current_commit = line[7:]
        current_commit_status = ""
        raw_commits_info[current_commit] = {}
    if current_commit == None:
        continue

    if line == "":
        continue
    elif line.startswith("parent "):
        if "parent" not in raw_commits_info[current_commit]:
            raw_commits_info[current_commit]["parent"] = []
        raw_commits_info[current_commit]["parent"].append(line[7:])
    elif line.startswith("author "):
        raw_commits_info[current_commit].update({ "author": line[7:].split(' <', 1)[0] })
    elif line.startswith("committer "):
        raw_commits_info[current_commit].update({ "committer": line[10:].split(' <', 1)[0] })
    elif line.startswith("gpgsig"):
        current_commit_status = "gpgsig"
    elif line.find("-----END PGP SIGNATURE-----") != -1:
        current_commit_status = ""
    elif line.startswith("commit "):
        pass
    elif line.startswith("tree "):
        pass
    elif current_commit_status == "":
        if "message" in raw_commits_info[current_commit] and not raw_commits_info[current_commit]["message"].startswith("Merge"):
            continue
        raw_commits_info[current_commit]["message"] = line

commits = {}

def search_commits(commit_hash: str, to):
    if commit_hash not in raw_commits_info:
        return
    raw_commit_info = raw_commits_info[commit_hash]
    if "visited" in raw_commit_info:
        return
    raw_commit_info.update({"visited": True})
    commit_key = raw_commit_info["message"] + f" @[{raw_commit_info['author']}]"
    to[commit_key] = {}
    search_commits(raw_commit_info["parent"][0], to)
    if len(raw_commit_info["parent"]) == 2:
        search_commits(raw_commit_info["parent"][1], to[commit_key])

search_commits([x for x in raw_commits_info.keys()][0], commits)

# print(json.dumps(commits, ensure_ascii=False, indent=2))
# exit(0)

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
            elif x.startswith("feat"):
                sorted_commits["feat"][x] = commits[x]
            elif x.startswith("perf"):
                sorted_commits["perf"][x] = commits[x]
            elif x.startswith("fix"):
                sorted_commits["fix"][x] = commits[x]
            elif x.find("新增") != -1:
                sorted_commits["feat"][x] = commits[x]
            elif x.find("改进") != -1 or x.find("优化") != -1:
                sorted_commits["perf"][x] = commits[x]
            elif x.find("修复") != -1:
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
            contributor_re = re.search(r"@\[.*\]", x)
            if not contributor_re:
                print(f"Warning: `{x}` has no contributor!")
            else:
                contributor = contributor_re.group()
                if contributor not in contributors:
                    print(f"Warning: New contributor `{contributor}`.")
                else:
                    contributor = contributors[contributor]
                commit_message = re.sub(r"@\[.*\]", "", commit_message)

            mes, ctrs = print_commits(commits[x], indent + "   ", False)
            ret_message += indent + "- " + commit_message

            if ctrs.count(contributor) == 0:
                ret_message += contributor
                if ret_contributor.count(contributor) == 0:
                    ret_contributor.append(contributor)
            for ctr in ctrs:
                ret_message += " " + ctr
                if ret_contributor.count(ctr) == 0:
                    ret_contributor.append(ctr)
            ret_message += "\n" + mes

    return ret_message, ret_contributor

res = print_commits(commits)
nightly = os.popen("git describe --tags").read()[:-1]

with open("../changelog.md", "w", encoding="utf8") as f:
    f.write("## " + nightly + "\n" + res[0])
