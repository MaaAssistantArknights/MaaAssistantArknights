from typing import List, Dict

def select_targets(scan_results: List[Dict]) -> List[Dict]:
    """
    极简 triage：有输出就取前 N 个尝试。后续可加打分：可信度、变更规模、是否高风险等。
    """
    targets = []
    for r in scan_results:
        if r.get("output"):
            targets.append({"source": r["tool"], "evidence": r["output"][:8000]})
    return targets[:2]
