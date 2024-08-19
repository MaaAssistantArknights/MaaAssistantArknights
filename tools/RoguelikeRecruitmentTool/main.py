from pathlib import Path

from roguelike_recruitment_tool import RecruitmentTool


if __name__ == "__main__":
    resource_path = Path(__file__).resolve().parent.parent.parent / "resource"

    app = RecruitmentTool(resource_path, [])
    app.exec()
