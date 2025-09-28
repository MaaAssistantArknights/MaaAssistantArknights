import os
import subprocess
from git import Repo
from datetime import datetime

def ensure_repo(repo_url: str, workdir: str):
    os.makedirs(workdir, exist_ok=True)
    repo_path = os.path.join(workdir, "repo")
    if not os.path.exists(repo_path):
        Repo.clone_from(repo_url, repo_path)
    else:
        repo = Repo(repo_path)
        repo.git.fetch("--all", "--prune")
        # 确保工作树干净
        repo.git.reset("--hard")
    return repo_path

def create_work_branch(repo_path: str, base_branch: str, branch_prefix: str):
    repo = Repo(repo_path)
    repo.git.checkout(base_branch)
    ts = datetime.utcnow().strftime("%Y%m%d%H%M%S")
    new_branch = f"{branch_prefix}/{ts}"
    repo.git.checkout("-b", new_branch)
    return new_branch

def git_apply_patch(repo_path: str, patch_text: str) -> bool:
    try:
        subprocess.run(
            ["git", "apply", "--index", "-p0", "-"],
            input=patch_text.encode("utf-8"),
            cwd=repo_path,
            check=True,
        )
        return True
    except subprocess.CalledProcessError:
        return False

def git_commit_and_push(repo_path: str, message: str, author_name: str, author_email: str, remote: str | None):
    repo = Repo(repo_path)
    repo.index.commit(message, author=f"{author_name} <{author_email}>")
    if remote:
        repo.git.push(remote, repo.active_branch.name)
