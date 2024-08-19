from typing import Optional

from PyQt5.QtWidgets import QDialog, QFormLayout, QLineEdit, QPushButton, QWidget

from roguelike.recruitment import Group


class GroupEditDialog(QDialog):
    def __init__(self, group: Group, parent: Optional[QWidget] = None) -> None:
        super().__init__(parent)

        self.setModal(True)
        self.setWindowTitle("Edit Group")

        self.layout = QFormLayout(self)
        self.setLayout(self.layout)

        self.name_edit = QLineEdit(group.name, self)
        self.doc_edit = QLineEdit(group.doc, self)

        self.save_button = QPushButton("Save", self)
        self.save_button.clicked.connect(lambda: self.accept())

        self.layout.addRow("Name:", self.name_edit)
        self.layout.addRow("Description:", self.doc_edit)
        self.layout.addWidget(self.save_button)

    def get_input(self):
        return self.name_edit.text(), self.doc_edit.text()
