import os, subprocess

def ensure_branch(cfg):
    subprocess.run("git fetch origin", shell=True)
    subprocess.run(f"git checkout {cfg['base_branch']}", shell=True)
    subprocess.run("git pull", shell=True)
    subprocess.run(f"git checkout -B {cfg['work_branch']}", shell=True)

def create_patch_commit(cfg, diff_path, message):
    if not os.path.exists(diff_path):
        print("No diff file")
        return False
    p = subprocess.run(f"git apply --index {diff_path}", shell=True)
    if p.returncode != 0:
        return False
    subprocess.run(f'git commit -m "{message}"', shell=True)
    subprocess.run(f"git push -u origin {cfg['work_branch']}", shell=True)
    return True

def open_pr(cfg, title):
    token = os.getenv(cfg["github"]["token_env"])
    if not token:
        print("No GITHUB_TOKEN env; skip PR")
        return
    cmd = f'gh pr create --title "{title}" --body "Automated fix by Agent" --base {cfg["github"]["pr_base"]} --head {cfg["work_branch"]}'
    subprocess.run(cmd, shell=True)