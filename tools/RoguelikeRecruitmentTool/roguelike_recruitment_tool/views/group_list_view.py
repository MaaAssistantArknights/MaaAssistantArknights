from typing import Optional

from PyQt5.QtCore import Qt
from PyQt5.QtGui import QCursor, QDrag, QKeyEvent
from PyQt5.QtWidgets import QAction, QDialog, QListView, QMenu, QToolTip

from ..common import DocRole
from ..delegates import EditableDelegate
from ..dialogs import GroupEditDialog

from roguelike.recruitment import new_group


class GroupListView(QListView):
    def __init__(self, parent=None):
        super().__init__(parent)

        self.setSelectionMode(QListView.SingleSelection)

        # right-click
        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self.show_context_menu)
        # double-click
        self.setEditTriggers(QListView.DoubleClicked)
        self.setItemDelegate(EditableDelegate(self))
        # drag & drop
        self.setDragEnabled(True)
        self.setAcceptDrops(True)
        self.setDragDropMode(QListView.InternalMove)
        self.setDefaultDropAction(Qt.MoveAction)

    # ———————— Q-key —————————————————————————————————————————————————

    def keyPressEvent(self, event: Optional[QKeyEvent]):
        super().keyPressEvent(event)

        if event.key() == Qt.Key_Q:
            index = self.indexAt(self.viewport().mapFromGlobal(QCursor.pos()))
            if index.isValid():
                content = f"doc: {self.model().data(index, DocRole).value()}"
                QToolTip.showText(QCursor.pos(), content)

    # ———————— right-click ———————————————————————————————————————————

    def show_context_menu(self, pos):
        if self.model().get_group_list() is None:
            return

        context_menu = QMenu(self)

        selected_indices = self.selectedIndexes()
        if selected_indices:
            selected_group_index = selected_indices[0].row()

            insert_above_action = QAction("Insert Above", self)
            insert_above_action.triggered.connect(lambda: self.create_group(selected_group_index))
            context_menu.addAction(insert_above_action)

            insert_below_action = QAction("Insert Below", self)
            insert_below_action.triggered.connect(lambda: self.create_group(selected_group_index + 1))
            context_menu.addAction(insert_below_action)

            update_action = QAction("Edit", self)
            update_action.triggered.connect(lambda: self.update_group(selected_group_index))
            context_menu.addAction(update_action)

            delete_action = QAction("Delete", self)
            delete_action.triggered.connect(lambda: self.delete_group(selected_group_index))
            context_menu.addAction(delete_action)
        else:
            insert_action = QAction("Insert", self)
            insert_action.triggered.connect(lambda: self.create_group(0))
            context_menu.addAction(insert_action)

        context_menu.exec_(self.viewport().mapToGlobal(pos))

    def create_group(self, index: int) -> None:
        group_list = self.model().get_group_list()
        group_list.insert(index, new_group())
        self.model().set_group_list(group_list)
        self.selectionModel().select(self.model().index(index), self.selectionModel().Select)

    def update_group(self, index: int) -> None:
        group_list = self.model().get_group_list()
        dialog = GroupEditDialog(group_list[index])
        if dialog.exec_() == QDialog.Accepted:
            new_name, new_doc = dialog.get_input()
            self.model().setData(self.model().index(index), new_name, Qt.EditRole)
            self.model().setData(self.model().index(index), new_doc, DocRole)

    def delete_group(self, index: int) -> None:
        group_list = self.model().get_group_list()
        del group_list[index]
        self.model().set_group_list(group_list)

    # ———————— drag & drop ———————————————————————————————————————————

    def startDrag(self, supportedActions):
        drag = QDrag(self)
        mime_data = self.model().mimeData(self.selectedIndexes())
        drag.setMimeData(mime_data)
        drag.exec_(Qt.MoveAction)

    def dropEvent(self, event):
        super().dropEvent(event)
        self.selectionModel().clearSelection()
        self.selectionModel().select(self.indexAt(event.pos()), self.selectionModel().Select)
