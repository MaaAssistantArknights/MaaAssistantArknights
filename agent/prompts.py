import json

def build_analysis_prompt(evidence):
    return f"""
项目分析证据（精简）：
- 构建是否成功: {evidence['build']['ok']}
- CTest是否通过: {evidence['ctest']['ok']}
- CTest摘要Top: {json.dumps(evidence['ctest']['summary'][:5], ensure_ascii=False)}
- clang-tidy fixes数量: {len(evidence['clang_tidy']['fixes'])}
- cppcheck问题数量: {len(evidence['cppcheck']['issues'])}

任务：
1) 结合构建错误/测试失败/静态分析证据，定位最可能的根因（指到文件/函数/行）。
2) 给出风险最小的修复方向（逐步计划），考虑跨平台与线程安全。
3) 列出需要修改的文件清单与变更要点。
输出格式：JSON，包含 keys: root_cause, plan_steps, file_changes。
"""

def build_plan_prompt(plan_json_str):
    return f"""
这是修复计划（JSON）：
{plan_json_str}

请基于计划生成最小改动的代码补丁，确保：
- 不破坏公共接口；
- 优先修复能导致编译失败/测试失败/高严重的静态问题；
- 遵守现有代码风格（clang-format）。

请输出unified diff（git diff -U3风格），仅包含diff内容。
"""

def build_patch_prompt():
    return "注意：如果不需要修改的文件不要出现在diff中。"
