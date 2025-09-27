import os, subprocess, xml.etree.ElementTree as ET, yaml

def run_clang_tidy(cfg):
    build_dir = cfg["tools"]["build_dir"]
    export_fixes = cfg["tools"]["clang_tidy_fixes"]
    tidy_checks = cfg["tools"]["clang_tidy_checks"]
    src_dir = cfg["repo_path"]
    # 只对 src 目录/指定路径跑，避免全仓库扫描过慢；可在 config.yaml 中调整
    targets = cfg["tools"]["clang_tidy_targets"]
    try:
        # 清理旧fixes
        if os.path.exists(export_fixes):
            os.remove(export_fixes)
        cmd = f"clang-tidy -p {build_dir} {' '.join(targets)} -checks='{tidy_checks}' -export-fixes={export_fixes} || true"
        p = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=7200)
        ok = True  # clang-tidy 常返回非0，这里不以返回码判定
        return ok, (p.stdout + "\n" + p.stderr)
    except Exception as e:
        return False, str(e)

def parse_clang_tidy_fixes(cfg):
    path = cfg["tools"]["clang_tidy_fixes"]
    fixes = []
    if not os.path.exists(path):
        return fixes
    try:
        data = yaml.safe_load(open(path,"r",encoding="utf-8"))
        for d in data.get("Diagnostics", []):
            fixes.append({
                "message": d.get("DiagnosticMessage", {}).get("Message"),
                "file": d.get("DiagnosticMessage", {}).get("FilePath"),
                "replacements": len(d.get("Replacements", []))
            })
    except Exception:
        pass
    fixes.sort(key=lambda x: -(x["replacements"] or 0))
    return fixes

def run_cppcheck(cfg):
    build_dir = cfg["tools"]["build_dir"]
    xml_out = cfg["tools"]["cppcheck_xml"]
    includes = cfg["tools"]["cppcheck_includes"]
    enable = cfg["tools"]["cppcheck_enable"]
    ignore = cfg["tools"]["cppcheck_ignore"]
    src = cfg["repo_path"]
    try:
        # 允许失败但产出报告
        cmd = f"cppcheck --project={build_dir}/compile_commands.json --enable={enable} --xml 2>{xml_out}"
        if includes:
            cmd += " " + " ".join([f"-I {p}" for p in includes])
        for ig in ignore:
            cmd += f" --suppress={ig}"
        p = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=7200)
        ok = True
        return ok, (p.stdout + "\n" + p.stderr)
    except Exception as e:
        return False, str(e)

def parse_cppcheck_xml(cfg):
    path = cfg["tools"]["cppcheck_xml"]
    items = []
    if not os.path.exists(path):
        return items
    try:
        tree = ET.parse(path)
        root = tree.getroot()
        for err in root.findall(".//error"):
            items.append({
                "id": err.get("id"),
                "severity": err.get("severity"),
                "msg": err.get("msg"),
                "file": (err.find("location") or {}).get("file") if err.find("location") is not None else None,
                "line": (err.find("location") or {}).get("line") if err.find("location") is not None else None,
            })
    except Exception:
        pass
    # 简单排序：严重程度优先
    sev_order = {"error":0,"warning":1,"style":2,"performance":3,"portability":4}
    items.sort(key=lambda x: sev_order.get(x["severity"], 5))
    return items