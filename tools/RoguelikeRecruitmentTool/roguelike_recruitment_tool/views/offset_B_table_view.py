from PyQt5.QtCore import QItemSelection, QModelIndex, Qt
from PyQt5.QtGui import QDrag
from PyQt5.QtWidgets import QAction, QMenu

from roguelike.recruitment import new_collection_priority_offset

from .table_view import TableView


class OffsetBTableView(TableView):
    def __init__(self, parent=None):
        super().__init__(parent)

        # right-click
        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self.show_context_menu)
        # drag & drop
        self.setDragEnabled(True)
        self.setAcceptDrops(True)
        self.setDropIndicatorShown(True)
        self.setDragDropMode(TableView.InternalMove)
        self.setDefaultDropAction(Qt.MoveAction)

    # ———————— right-click ———————————————————————————————————————————

    def show_context_menu(self, pos):
        if self.model().get_offset_list() is None:
            return

        context_menu = QMenu(self)

        selected_indices = self.selectedIndexes()
        if selected_indices:
            selected_group_index = selected_indices[0].row()

            insert_above_action = QAction("Insert Above", self)
            insert_above_action.triggered.connect(lambda: self.create_offset(selected_group_index))
            context_menu.addAction(insert_above_action)

            insert_below_action = QAction("Insert Below", self)
            insert_below_action.triggered.connect(lambda: self.create_offset(selected_group_index + 1))
            context_menu.addAction(insert_below_action)

            delete_action = QAction("Delete", self)
            delete_action.triggered.connect(lambda: self.delete_offset(selected_group_index))
            context_menu.addAction(delete_action)
        else:
            insert_action = QAction("Insert", self)
            insert_action.triggered.connect(lambda: self.create_offset(0))
            context_menu.addAction(insert_action)

        context_menu.exec_(self.viewport().mapToGlobal(pos))

    def create_offset(self, index: int) -> None:
        oper_list = self.model().get_oper_list()
        oper_list.insert(index, new_collection_priority_offset())
        self.model().set_oper_list(oper_list)
        self.selectionModel().select(self.model().index(index), self.selectionModel().Select)

    def delete_offset(self, index: int) -> None:
        offset_list = self.model().get_offset_list()
        del offset_list[index]
        self.model().set_offset_list(offset_list)

    # ———————— drag & drop ———————————————————————————————————————————

    def startDrag(self, supportedActions):
        drag = QDrag(self)
        mime_data = self.model().mimeData(self.selectedIndexes())
        drag.setMimeData(mime_data)
        drag.exec_(Qt.MoveAction)

    def dropEvent(self, event):
        self.model().dropMimeData(event.mimeData(), event.proposedAction(), self.indexAt(event.pos()).row(),
                                  self.indexAt(event.pos()).column(), QModelIndex())

        self.selectionModel().clearSelection()
        selected_row: int = self.indexAt(event.pos()).row()
        new_selection_top_left: QModelIndex = self.model().index(selected_row, 0)
        new_selection_bottom_right: QModelIndex = self.model().index(selected_row, self.model().columnCount() - 1)
        new_selection: QItemSelection = QItemSelection(new_selection_top_left, new_selection_bottom_right)
        self.selectionModel().select(new_selection, self.selectionModel().Select)
