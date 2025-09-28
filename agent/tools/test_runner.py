import subprocess

def run_tests(test_cmds, cwd):
    if not test_cmds:
        print("[test] no tests configured, treat as pass")
        return True
    ok = True
    for cmd in test_cmds:
        print(f"[test] running: {cmd}")
        ret = subprocess.run(cmd, cwd=cwd, shell=True)
        if ret.returncode != 0:
            ok = False
            break
    return ok
