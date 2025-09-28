import os, subprocess, glob

def configure_and_build(cfg):
    build_dir = cfg["tools"]["build_dir"]
    src_dir = cfg["repo_path"]
    cmake_config = cfg["tools"]["cmake_config"]
    cmake_build = cfg["tools"]["cmake_build"]
    try:
        p1 = subprocess.run(f"cmake -S {src_dir} -B {build_dir} {cmake_config}",
                            shell=True, capture_output=True, text=True, timeout=3600)
        p2 = subprocess.run(f"cmake --build {build_dir} {cmake_build}",
                            shell=True, capture_output=True, text=True, timeout=7200)
        ok = (p1.returncode == 0) and (p2.returncode == 0)
        logs = (p1.stdout + "\n" + p1.stderr + "\n" + p2.stdout + "\n" + p2.stderr)[-200000:]
        return ok, logs
    except Exception as e:
        return False, str(e)

def run_ctest(cfg):
    build_dir = cfg["tools"]["build_dir"]
    try:
        p = subprocess.run(f"ctest --test-dir {build_dir} --output-on-failure",
                           shell=True, capture_output=True, text=True, timeout=3600)
        ok = p.returncode == 0
        logs = (p.stdout + "\n" + p.stderr)[-200000:]
        return ok, logs
    except Exception as e:
        return False, str(e)

def summarize_ctest(cfg):
    build_dir = cfg["tools"]["build_dir"]
    ctest_log = os.path.join(build_dir, "Testing", "Temporary", "LastTest.log")
    items = []
    if os.path.exists(ctest_log):
        try:
            with open(ctest_log,"r",encoding="utf-8",errors="ignore") as f:
                lines = f.readlines()[-200:]
            for ln in lines:
                if "Failed" in ln or "FAILED" in ln or "error" in ln.lower():
                    items.append(ln.strip())
        except Exception:
            pass
    return items[:50]
