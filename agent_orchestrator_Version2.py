import os, json
from tools.build_and_test import configure_and_build, run_ctest, summarize_ctest
from tools.static_analyzers import run_clang_tidy, parse_clang_tidy_fixes, run_cppcheck, parse_cppcheck_xml
from tools.git_tools import ensure_branch, create_patch_commit, open_pr
from agent.prompts import build_analysis_prompt, build_plan_prompt, build_patch_prompt
from agent.simple_llm import call_llm

class Orchestrator:
    def __init__(self, cfg):
        self.cfg = cfg
        self.repo_path = cfg["repo_path"]

    def analyze(self):
        os.makedirs(self.cfg["tools"]["build_dir"], exist_ok=True)
        print("[Agent] CMake configure & build...")
        build_ok, build_logs = configure_and_build(self.cfg)

        print("[Agent] CTest (if available)...")
        test_ok, test_logs = run_ctest(self.cfg)
        ctest_summary = summarize_ctest(self.cfg)

        print("[Agent] clang-tidy...")
        tidy_ok, _ = run_clang_tidy(self.cfg)
        tidy_fixes = parse_clang_tidy_fixes(self.cfg)

        print("[Agent] cppcheck...")
        cpp_ok, _ = run_cppcheck(self.cfg)
        cpp_issues = parse_cppcheck_xml(self.cfg)

        evidence = {
            "build": {"ok": build_ok, "raw": build_logs[-4000:]},
            "ctest": {"ok": test_ok, "summary": ctest_summary, "raw": test_logs[-4000:]},
            "clang_tidy": {"ok": tidy_ok, "fixes": tidy_fixes[:50]},
            "cppcheck": {"ok": cpp_ok, "issues": cpp_issues[:100]},
        }
        with open("agent_artifacts_analysis.json", "w", encoding="utf-8") as f:
            json.dump(evidence, f, ensure_ascii=False, indent=2)
        print("[Agent] Analysis done -> agent_artifacts_analysis.json")
        return evidence

    def plan(self):
        if not os.path.exists("agent_artifacts_analysis.json"):
            evidence = self.analyze()
        else:
            evidence = json.load(open("agent_artifacts_analysis.json","r",encoding="utf-8"))

        prompt = build_analysis_prompt(evidence)
        plan = call_llm(self.cfg, prompt, sys_hint="你是资深C++/视觉工程师，请输出修复计划（JSON）。")
        with open("agent_plan.json", "w", encoding="utf-8") as f:
            f.write(plan)
        print("[Agent] Plan -> agent_plan.json")
        return plan

    def fix(self):
        if not os.path.exists("agent_plan.json"):
            self.plan()
        plan = open("agent_plan.json","r",encoding="utf-8").read()
        patch_prompt = build_plan_prompt(plan) + "\n\n" + build_patch_prompt()
        diff = call_llm(self.cfg, patch_prompt, sys_hint="请仅输出unified diff，尽量最小化修改。")
        open("agent_patch.diff","w",encoding="utf-8").write(diff)
        print("[Agent] Patch diff -> agent_patch.diff")

        if self.cfg["policy"]["auto_apply_patch"]:
            ensure_branch(self.cfg)
            ok = create_patch_commit(self.cfg, "agent_patch.diff", "[Agent] apply fix")
            if not ok:
                print("[Agent] Patch apply failed.")
                return False

            print("[Agent] Re-validating...")
            _, _ = configure_and_build(self.cfg)
            test_ok, _ = run_ctest(self.cfg)
            tidy_ok, _ = run_clang_tidy(self.cfg)
            cpp_ok, _ = run_cppcheck(self.cfg)

            if test_ok and tidy_ok and cpp_ok and self.cfg["policy"]["auto_open_pr"]:
                open_pr(self.cfg, title=self.cfg["github"]["pr_title_prefix"] + " Auto fix")

            print(f"[Agent] Validate: tests={test_ok}, clang-tidy={tidy_ok}, cppcheck={cpp_ok}")
        return True

    def full(self):
        self.analyze()
        self.plan()
        self.fix()