import os
import yaml
from agent.tools.git_utils import ensure_repo, create_work_branch, git_apply_patch, git_commit_and_push
from agent.tools.static_scan import run_scans
from agent.planner import select_targets
from agent.repairer import propose_patch
from agent.validator import validate_patch

def load_config():
    with open("agent/config.yaml", "r", encoding="utf-8") as f:
        return yaml.safe_load(f)

def main():
    cfg = load_config()
    repo_path = ensure_repo(cfg["repo_url"], cfg["workdir"])

    # 静态扫描
    scan_results = run_scans(cfg["scan"], repo_path)
    targets = select_targets(scan_results)
    if not targets:
        print("[agent] no actionable findings, exiting.")
        return

    # 建分支
    branch = create_work_branch(repo_path, cfg["default_base_branch"], cfg["branch_prefix"])
    print(f"[git] working on branch: {branch}")

    # 尝试修复
    attempts = 0
    for t in targets:
        if attempts >= cfg["llm"].get("max_patch_attempts", 2):
            break
        attempts += 1
        print(f"[agent] attempting fix from {t['source']}")
        patch = propose_patch(t["evidence"], repo_path, cfg["llm"])
        if not patch or "diff" not in patch:
            print("[patch] not a unified diff, skip")
            continue
        if not git_apply_patch(repo_path, patch):
            print("[patch] failed to apply, skip")
            continue

        ok, why = validate_patch(cfg, repo_path)
        if ok:
            msg = f"fix: auto patch based on {t['source']} scan ({why})"
            git_commit_and_push(repo_path, msg, cfg["git"]["author_name"], cfg["git"]["author_email"], cfg["git"].get("push_remote"))
            print("[agent] fix validated and pushed. Open a PR if desired.")
            return
        else:
            print(f"[agent] validation failed: {why}, reverting")
            os.system(f"git -C {repo_path} reset --hard")

    print("[agent] no successful patch in attempts.")

if __name__ == "__main__":
    main()
