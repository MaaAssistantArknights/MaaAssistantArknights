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

latest = os.popen("git describe --abbrev=0 --tags").read()[:-1]

print("From:", latest, "\n")

git_pretty_command = rf'git log {latest}..HEAD --pretty=format:"%C(yellow)%d%Creset %s %C(bold blue)@%an%Creset (%Cgreen%h%Creset)" --graph'
os.system(git_pretty_command)

git_command = rf'git log {latest}..HEAD --pretty=format:"%s %C(bold blue)@[%an%Creset]" --graph'
with os.popen(git_command) as fp: bf = fp._stream.buffer.read()
try: gitlogs = bf.decode().strip()
except: gitlogs = bf.decode("gbk").strip()

commits = {}
now_commit = [commits]
lastpos = -2

for line in gitlogs.split("\n"):
    star_pos = line.find("*")
    if star_pos == -1:
        continue
    commit_log_re = re.search(r"[a-z]", line, re.I)
    if not commit_log_re:
        continue
    commit_log = line[commit_log_re.start():]
    if commit_log.startswith("Merge"):
        continue

    while star_pos <= lastpos:
        if lastpos - star_pos == 1:
            print("Fatal error!")
            exit(0)
        now_commit.pop()
        lastpos -= 2

    if star_pos > lastpos:
        if star_pos - lastpos != 2:
            print(f"error!({line})")
            exit(0)
        now_commit[-1][commit_log] = {}
        now_commit.append(now_commit[-1][commit_log])
        lastpos += 2

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

# sorted_commits = {}
# for x in sorted([x for x in commits.keys()]):
#     sorted_commits[x] = commits[x]

# print(json.dumps(sorted_commits, indent=2, ensure_ascii=False))
