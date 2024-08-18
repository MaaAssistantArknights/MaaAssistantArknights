from PyQt5.QtCore import QAbstractTableModel, QModelIndex, QVariant, Qt

from roguelike.recruitment import Oper

from ..common import DescriptionRole, parse_field


class OperInfoTableModel(QAbstractTableModel):
    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._oper: Oper | None = None
        self._field_list: list[str] = list(Oper.model_fields.keys())[: -2]

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:
        if self._oper is None:
            return 0
        return len(self._field_list)

    def columnCount(self, parent: QModelIndex = QModelIndex()) -> int:
        if self._oper is None:
            return 0
        return 2

    def data(self, index: QModelIndex, role: int = Qt.DisplayRole) -> QVariant:
        if not index.isValid():
            return QVariant()
        if self._oper is None:
            return QVariant()
        if role == Qt.DisplayRole:
            field = self._field_list[index.row()]
            if index.column() == 0:
                return QVariant(field)
            elif index.column() == 1:
                return QVariant(str(getattr(self._oper, field)))
        elif role == Qt.EditRole:
            field = self._field_list[index.row()]
            if index.column() == 1:
                return QVariant(str(getattr(self._oper, field)))
        elif role == DescriptionRole:
            field = self._field_list[index.row()]
            if index.column() == 0:
                return QVariant(Oper.model_fields[field].description)
        return QVariant()

    def headerData(self, section, orientation, role=Qt.DisplayRole):
        if role == Qt.DisplayRole:
            if orientation == Qt.Horizontal:
                if section == 0:
                    return QVariant("Field")
                elif section == 1:
                    return QVariant("Value")
            else:
                return QVariant(f"{section + 1}")
        return QVariant()

    def flags(self, index):
        default_flags = super().flags(index)
        if index.isValid() and index.column() == 1:
            return default_flags | Qt.ItemIsEditable
        else:
            return default_flags

    def set_oper(self, oper: Oper | None):
        self.beginResetModel()
        self._oper = oper
        self.endResetModel()

    # ———————— double-click ——————————————————————————————————————————

    def setData(self, index, value, role=Qt.EditRole):
        if index.isValid():
            if role == Qt.EditRole and index.column() == 1:
                field = self._field_list[index.row()]
                field_type = Oper.__annotations__[field]
                field_value = parse_field(value, field_type)
                if field_value is not None:
                    setattr(self._oper, field, field_value)
                    self.dataChanged.emit(index, index, [role])
                    return True
        return False
