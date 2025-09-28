import subprocess

def run_commands(cmds, cwd):
    for cmd in cmds:
        print(f"[build] running: {cmd}")
        ret = subprocess.run(cmd, cwd=cwd, shell=True)
        if ret.returncode != 0:
            return False
    return True

def build_project(config, repo_path):
    if not run_commands(config.get("prepare", []), repo_path):
        return False
    return run_commands(config.get("commands", []), repo_path)
