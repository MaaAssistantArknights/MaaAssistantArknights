from pathlib import Path

from PyQt5.QtWidgets import QApplication

from roguelike.config import Theme
from roguelike.recruitment import Configuration
from .main_window import MainWindow


class RecruitmentTool(QApplication):
    VERSION = "0.0.1-pre-alpha-4"

    def __init__(self, resource_path: Path, argv: list[str]) -> None:
        super().__init__(argv)
        self.configurations: dict[Theme, Configuration] = {}
        self.main_window = MainWindow(self.configurations)

        self.main_window.setFixedSize(800, 480)
        self.main_window.version_label.setText(f"Version: {RecruitmentTool.VERSION}")
        self.main_window.resource_dir_line_widget.setText(str(resource_path))

        self.main_window.show()
