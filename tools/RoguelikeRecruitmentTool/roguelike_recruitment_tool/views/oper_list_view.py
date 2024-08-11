from typing import Optional

from PyQt5.QtCore import Qt
from PyQt5.QtGui import QCursor, QDrag, QKeyEvent
from PyQt5.QtWidgets import QAction, QApplication, QListView, QMenu, QToolTip

from pydantic import ValidationError

from roguelike.recruitment import Oper, new_oper

from ..common import DocRole


class OperListView(QListView):
    def __init__(self, parent=None):
        super().__init__(parent)

        self.setSelectionMode(QListView.SingleSelection)

        # right-click
        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self.show_context_menu)
        # drag & drop
        self.setDragEnabled(True)
        self.setAcceptDrops(True)
        self.setDragDropMode(QListView.InternalMove)
        self.setDefaultDropAction(Qt.MoveAction)

    # ———————— Q-key —————————————————————————————————————————————————

    def keyPressEvent(self, event: Optional[QKeyEvent]):
        if event.key() == Qt.Key_Q:
            index = self.indexAt(self.viewport().mapFromGlobal(QCursor.pos()))
            if index.isValid():
                content = f"doc: {self.model().data(index, DocRole).value()}"
                QToolTip.showText(QCursor.pos(), content)
        super().keyPressEvent(event)

    # ———————— right-click ———————————————————————————————————————————

    def show_context_menu(self, pos):
        if self.model().get_oper_list() is None:
            return

        context_menu = QMenu(self)

        selected_indices = self.selectedIndexes()
        if selected_indices:
            selected_oper_index = selected_indices[0].row()

            insert_above_action = QAction("Insert Above", self)
            insert_above_action.triggered.connect(lambda: self.create_oper(selected_oper_index))
            context_menu.addAction(insert_above_action)

            insert_below_action = QAction("Insert Below", self)
            insert_below_action.triggered.connect(lambda: self.create_oper(selected_oper_index + 1))
            context_menu.addAction(insert_below_action)

            copy_action = QAction("Copy", self)
            copy_action.triggered.connect(lambda: self.copy_oper(selected_oper_index))
            context_menu.addAction(copy_action)

            if QApplication.clipboard().text():
                paste_above_action = QAction("Paste Above", self)
                paste_above_action.triggered.connect(lambda: self.paste_oper(selected_oper_index))
                context_menu.addAction(paste_above_action)

                paste_below_action = QAction("Paste Below", self)
                paste_below_action.triggered.connect(lambda: self.paste_oper(selected_oper_index + 1))
                context_menu.addAction(paste_below_action)

            delete_action = QAction("Delete", self)
            delete_action.triggered.connect(lambda: self.delete_oper(selected_oper_index))
            context_menu.addAction(delete_action)
        else:
            insert_action = QAction("Insert", self)
            insert_action.triggered.connect(lambda: self.create_oper(0))
            context_menu.addAction(insert_action)

            if QApplication.clipboard().text():
                paste_action = QAction("Paste", self)
                paste_action.triggered.connect(lambda: self.paste_oper(selected_oper_index))
                context_menu.addAction(paste_action)

        context_menu.exec_(self.viewport().mapToGlobal(pos))

    def create_oper(self, index: int) -> None:
        oper_list = self.model().get_oper_list()
        oper_list.insert(index, new_oper())
        self.model().set_oper_list(oper_list)
        self.selectionModel().select(self.model().index(index), self.selectionModel().Select)

    def copy_oper(self, index: int) -> None:
        QApplication.clipboard().setText(Oper.oper2json(self.model().get_oper_list()[index]))

    def paste_oper(self, index: int) -> None:
        json_str = QApplication.clipboard().text()
        if not json_str:
            return
        oper_list = self.model().get_oper_list()
        try:
            pasted_oper: Oper = Oper.json2oper(json_str)
        except ValidationError:
            return
        oper_list.insert(index, pasted_oper)
        self.model().set_oper_list(oper_list)
        self.selectionModel().select(self.model().index(index), self.selectionModel().Select)

    def delete_oper(self, index: int) -> None:
        oper_list = self.model().get_oper_list()
        del oper_list[index]
        self.model().set_oper_list(oper_list)

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
