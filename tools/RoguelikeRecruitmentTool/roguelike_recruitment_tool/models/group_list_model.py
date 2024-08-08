import pickle
from typing import Callable

from PyQt5.QtCore import QAbstractListModel, QMimeData, QModelIndex, QVariant, Qt

from roguelike.recruitment import Group

from ..common import DocRole


class GroupListModel(QAbstractListModel):
    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._group_list: list[Group] | None = None
        self.extra_reset: Callable[[], None] = lambda: None

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:
        if self._group_list is None:
            return 0
        return len(self._group_list)

    def data(self, index: QModelIndex, role: int = Qt.DisplayRole) -> QVariant:
        if not index.isValid():
            return QVariant()
        if self._group_list is None:
            return QVariant()
        if role == Qt.DisplayRole:
            group: Group = self._group_list[index.row()]
            return QVariant(f"{group.name} ({len(group.opers)})")
        elif role == Qt.EditRole:
            group: Group = self._group_list[index.row()]
            return QVariant(group.name)
        elif role == DocRole:
            group: Group = self._group_list[index.row()]
            return QVariant(f"{group.doc}")
        return QVariant()

    def flags(self, index):
        default_flags = super().flags(index)
        if index.isValid():
            return default_flags | Qt.ItemIsEditable | Qt.ItemIsDragEnabled | Qt.ItemIsDropEnabled
        else:
            return default_flags | Qt.ItemIsDropEnabled

    def set_group_list(self, group_list: list[Group] | None) -> None:
        self.beginResetModel()
        self._group_list = group_list
        self.endResetModel()
        self.extra_reset()

    def get_group_list(self) -> list[Group] | None:
        return self._group_list

    # ———————— double-click ——————————————————————————————————————————

    def setData(self, index, value, role=Qt.EditRole):
        if index.isValid():
            if role == Qt.EditRole:
                self._group_list[index.row()].name = value
                self.dataChanged.emit(index, index, [role])
                return True
            elif role == DocRole:
                self._group_list[index.row()].doc = value
                self.dataChanged.emit(index, index, [role])
        return False

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
        self.beginMoveRows(QModelIndex(), src_row, src_row, QModelIndex(), row)
        self._group_list.insert(row, self._group_list.pop(src_row))
        self.endMoveRows()
        return True
