from .tools.build import build_project
from .tools.test_runner import run_tests

def validate_patch(config, repo_path):
    # 若未配置构建/测试，则直接视为通过（MVP）
    if not config["build"].get("commands"):
        return True, "no-build-mode"
    if not build_project(config["build"], repo_path):
        return False, "build failed"
    if not run_tests(config["build"].get("test_commands", []), repo_path):
        return False, "tests failed"
    return True, "ok"
