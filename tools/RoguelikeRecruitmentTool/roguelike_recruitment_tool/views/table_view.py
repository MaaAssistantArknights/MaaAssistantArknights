from typing import Optional

from PyQt5.QtCore import Qt
from PyQt5.QtGui import QCursor, QKeyEvent
from PyQt5.QtWidgets import QAbstractItemView, QHeaderView, QTableView, QToolTip

from ..common import DescriptionRole
from ..delegates import EditableDelegate


class TableView(QTableView):
    def __init__(self, parent=None):
        super().__init__(parent)

        self.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)

        # double-click
        self.setEditTriggers(QAbstractItemView.DoubleClicked)
        self.setItemDelegate(EditableDelegate(self))

    # ———————— Q-key —————————————————————————————————————————————————

    def keyPressEvent(self, event: Optional[QKeyEvent]):
        if event.key() == Qt.Key_Q:
            index = self.indexAt(self.viewport().mapFromGlobal(QCursor.pos()))
            if index.isValid():
                content = (f"{self.model().data(index, Qt.DisplayRole).value()}" +
                           (f"\n{self.model().data(index, DescriptionRole).value()}"
                            if self.model().data(index, DescriptionRole).value() is not None
                            else ""))
                QToolTip.showText(QCursor.pos(), content)
        super().keyPressEvent(event)
