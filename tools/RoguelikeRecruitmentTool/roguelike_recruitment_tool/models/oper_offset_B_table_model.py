import pickle

from PyQt5.QtCore import QAbstractTableModel, QMimeData, QModelIndex, QVariant, Qt

from roguelike.recruitment import CollectionPriorityOffset

from ..common import parse_field


class OperOffsetBTableModel(QAbstractTableModel):
    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._offset_list: list[CollectionPriorityOffset] | None = None
        self._field_list: list[str] = list(CollectionPriorityOffset.model_fields.keys())

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:
        if self._offset_list is None:
            return 0
        return len(self._offset_list)

    def columnCount(self, parent: QModelIndex = QModelIndex()) -> int:
        if self._offset_list is None:
            return 0
        return len(CollectionPriorityOffset.model_fields)

    def data(self, index: QModelIndex, role: int = Qt.DisplayRole) -> QVariant:
        if not index.isValid():
            return QVariant()
        if self._offset_list is None:
            return QVariant()
        if role == Qt.DisplayRole:
            offset = self._offset_list[index.row()]
            field = self._field_list[index.column()]
            return QVariant(getattr(offset, field))
        elif role == Qt.EditRole:
            offset = self._offset_list[index.row()]
            field = self._field_list[index.column()]
            return QVariant(str(getattr(offset, field)))
        return QVariant()

    def headerData(self, section, orientation, role=Qt.DisplayRole):
        if role == Qt.DisplayRole:
            if orientation == Qt.Horizontal:
                return QVariant(self._field_list[section])
            else:
                return QVariant(f"{section + 1}")
        return QVariant()

    def flags(self, index):
        default_flags = super().flags(index)
        if index.isValid():
            return default_flags | Qt.ItemIsEditable | Qt.ItemIsDragEnabled | Qt.ItemIsDropEnabled
        else:
            return default_flags | Qt.ItemIsDropEnabled

    def set_offset_list(self, offset_list: list[CollectionPriorityOffset] | None):
        self.beginResetModel()
        self._offset_list = offset_list
        self.endResetModel()

    def get_offset_list(self) -> list[CollectionPriorityOffset] | None:
        return self._offset_list

    # ———————— double-click ——————————————————————————————————————————

    def setData(self, index, value, role=Qt.EditRole):
        if index.isValid():
            if role == Qt.EditRole:
                offset = self._offset_list[index.row()]
                field = self._field_list[index.column()]
                field_type = RecruitPriorityOffset.__annotations__[field]
                field_value = parse_field(value, field_type)
                if field_value is not None:
                    setattr(offset, field, field_value)
                    self.dataChanged.emit(index, index, [role])
                    return True
        return False

# ———————— drag & drop ———————————————————————————————————————————

    def supportedDragActions(self):
        return Qt.MoveAction

    def supportedDropActions(self):
        return Qt.MoveAction

    def mimeTypes(self):
        return ["application/x-row"]

    def mimeData(self, indexes):
        mime_data = QMimeData()

        index = indexes[0]
        if index.isValid():
            mime_data.setData("application/x-row", pickle.dumps(index.row()))
        return mime_data

    def dropMimeData(self, data, action, row, column, parent):
        if action == Qt.IgnoreAction:
            return True
        if not data.hasFormat("application/x-row"):
            return False
        if row == -1:
            return False

        src_row = pickle.loads(data.data("application/x-row").data())
        self.layoutAboutToBeChanged.emit()
        self._offset_list.insert(row, self._offset_list.pop(src_row))
        self.layoutChanged.emit()
        return True
