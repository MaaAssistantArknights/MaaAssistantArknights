from PyQt5.QtCore import QItemSelection

from roguelike.recruitment import Configuration, Group, Oper

from .group_list_model import GroupListModel
from .oper_info_table_model import OperInfoTableModel
from .oper_list_model import OperListModel
from .oper_offset_A_table_model import OperOffsetATableModel
from .oper_offset_B_table_model import OperOffsetBTableModel


class VisualisationModel:
    def __init__(self, parent=None) -> None:
        self.group_list_model: GroupListModel = GroupListModel(parent)
        self.oper_list_model: OperListModel = OperListModel(parent)
        self.oper_info_model: OperInfoTableModel = OperInfoTableModel(parent)
        self.oper_offset_A_model: OperOffsetATableModel = OperOffsetATableModel(parent)
        self.oper_offset_B_model: OperOffsetBTableModel = OperOffsetBTableModel(parent)
        self.group_list_model.extra_reset = lambda: (self.oper_list_model.set_oper_list(None),
                                                     self.oper_info_model.set_oper(None),
                                                     self.oper_offset_A_model.set_offset_list(None),
                                                     self.oper_offset_B_model.set_offset_list(None))
        self.oper_list_model.extra_reset = lambda: (self.oper_info_model.set_oper(None),
                                                    self.oper_offset_A_model.set_offset_list(None),
                                                    self.oper_offset_B_model.set_offset_list(None))
        self._configuration: Configuration | None = None
        self._selected_group_index: int | None = None
        self._selected_oper_index: int | None = None

    def set_configuration(self, config: Configuration | None) -> None:
        self._configuration = config
        self._selected_group_index = None
        self._selected_oper_index = None
        self.group_list_model.set_group_list(self._configuration.priority if self._configuration is not None else None)

    def get_configuration(self) -> Configuration | None:
        return self._configuration

    def get_selected_group_index(self) -> int | None:
        return self._selected_group_index

    def get_selected_oper_index(self) -> int | None:
        return self._selected_oper_index

    def on_group_selection(self, selected: QItemSelection, deselected: QItemSelection = None) -> None:
        if not selected.indexes():
            return
        self._selected_group_index = selected.indexes()[0].row()
        self._selected_oper_index = None
        selected_group: Group = self._configuration.priority[self._selected_group_index]
        self.oper_list_model.set_oper_list(selected_group.opers)

    def on_oper_selection(self, selected: QItemSelection, deselected: QItemSelection = None) -> None:
        if not selected.indexes():
            return
        self._selected_oper_index = selected.indexes()[0].row()
        selected_group: Group = self._configuration.priority[self._selected_group_index]
        selected_oper: Oper = selected_group.opers[self._selected_oper_index]
        self.oper_info_model.set_oper(selected_oper)
        self.oper_offset_A_model.set_offset_list(selected_oper.recruit_priority_offsets)
        self.oper_offset_B_model.set_offset_list(selected_oper.collection_priority_offsets)
