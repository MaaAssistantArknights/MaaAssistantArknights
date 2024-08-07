from PyQt5.QtCore import QSize
from PyQt5.QtWidgets import QTabBar


class TabBar(QTabBar):
    def tabSizeHint(self, index):
        width = self.width() // self.count() if self.count() else 0
        height = QTabBar.tabSizeHint(self, index).height()
        return QSize(width, height)
