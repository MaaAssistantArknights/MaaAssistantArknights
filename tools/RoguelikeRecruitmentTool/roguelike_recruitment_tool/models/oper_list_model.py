import pickle
from typing import Callable

from PyQt5.QtCore import QAbstractListModel, QMimeData, QModelIndex, QVariant, Qt

from roguelike.recruitment import Oper

from ..common import DocRole


class OperListModel(QAbstractListModel):
    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._oper_list: list[Oper] | None = None
        self.extra_reset: Callable[[], None] = lambda: None

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:
        if self._oper_list is None:
            return 0
        return len(self._oper_list)

    def data(self, index: QModelIndex, role: int = Qt.DisplayRole) -> QVariant:
        if not index.isValid():
            return QVariant()
        if self._oper_list is None:
            return QVariant()
        if role == Qt.DisplayRole:
            oper: Oper = self._oper_list[index.row()]
            return QVariant(f"{oper.name} [{oper.skill}]")
        if role == DocRole:
            oper: Oper = self._oper_list[index.row()]
            return QVariant(f"{oper.doc}")
        return QVariant()

    def flags(self, index):
        default_flags = super().flags(index)
        if index.isValid():
            return default_flags | Qt.ItemIsDragEnabled | Qt.ItemIsDropEnabled
        else:
            return default_flags | Qt.ItemIsDropEnabled

    def set_oper_list(self, oper_list: list[Oper] | None) -> None:
        self.beginResetModel()
        self._oper_list = oper_list
        self.endResetModel()
        self.extra_reset()

    def get_oper_list(self) -> list[Oper] | None:
        return self._oper_list

    # ———————— drag & drop ———————————————————————————————————————————

    def supportedDragActions(self):
        return Qt.MoveAction

    def supportedDropActions(self):
        return Qt.MoveAction

    def mimeTypes(self):
        return ["application/x-custom-list-item"]

    def mimeData(self, indexes):
        mime_data = QMimeData()

        index = indexes[0]
        if index.isValid():
            mime_data.setData("application/x-custom-list-item", pickle.dumps(index.row()))
        return mime_data

    def dropMimeData(self, data, action, row, column, parent):
        if action == Qt.IgnoreAction:
            return True
        if not data.hasFormat("application/x-custom-list-item"):
            return False
        if row == -1:
            return False

        src_row = pickle.loads(data.data("application/x-custom-list-item").data())
        if action == Qt.MoveAction:
            self.beginMoveRows(QModelIndex(), src_row, src_row, QModelIndex(), row)
            self._oper_list.insert(row, self._oper_list.pop(src_row))
            self.endMoveRows()
            return True
        elif action == Qt.CopyAction:
            self.beginInsertRows(QModelIndex(), row, row)
            self._oper_list.insert(row, self._oper_list[src_row])
            self.endInsertRows()
            return True

        return False
