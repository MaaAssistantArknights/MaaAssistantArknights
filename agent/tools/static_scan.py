import subprocess
from typing import List, Dict

def _run(cmd, cwd: str):
    try:
        ret = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, shell=True)
        return ret.returncode, ret.stdout, ret.stderr
    except Exception as e:
        return 1, "", str(e)

def cppcheck(paths: List[str], exclude: List[str], cwd: str) -> List[Dict]:
    findings = []
    excl = " ".join([f"--suppress=*:{e}/*" for e in exclude])
    for p in paths:
        code, out, err = _run(f"cppcheck --enable=all --inconclusive --std=c++17 {excl} {p}", cwd)
        if out or err:
            findings.append({"tool": "cppcheck", "output": out + "\n" + err})
    return findings

def semgrep_run(config_path: str, paths: List[str], cwd: str) -> List[Dict]:
    findings = []
    code, out, err = _run(f"semgrep --config {config_path} {' '.join(paths)} --json", cwd)
    if out:
        findings.append({"tool": "semgrep", "output": out})
    return findings

def run_scans(config: dict, repo_path: str) -> List[Dict]:
    results = []
    if config.get("enable_cppcheck"):
        results += cppcheck(config.get("paths", ["."]), config.get("exclude", []), repo_path)
    if config.get("enable_semgrep"):
        results += semgrep_run(config.get("semgrep_config", "semgrep.yml"), config.get("paths", ["."]), repo_path)
    # clang-tidy 需 compile_commands.json，先不默认启用
    return results
