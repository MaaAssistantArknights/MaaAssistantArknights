from enum import IntEnum
from typing import Annotated, Any, List, get_args, get_origin

from PyQt5.QtCore import Qt


class RecruitmentItemDataRole(IntEnum):
    DocRole = Qt.UserRole + 1
    DescriptionRole = Qt.UserRole + 2


DocRole = RecruitmentItemDataRole.DocRole
DescriptionRole = RecruitmentItemDataRole.DescriptionRole

str_bool_dict = {"True": True, "False": False,
                 "true": True, "false": False,
                 "T": True, "F": False,
                 "t": True, "f": False,
                 "1": True, "0": False}


def parse_field(value: str, field_type: type) -> Any:
    print(get_args(field_type))
    if field_type is str or get_origin(field_type) is Annotated and get_args(field_type)[0] is str:
        field_value = value
    elif field_type is int or get_origin(field_type) is Annotated and get_args(field_type)[0] is int:
        try:
            field_value = int(value)
        except ValueError:
            field_value = None
    elif field_type is bool or get_origin(field_type) is Annotated and get_args(field_type)[0] is bool:
        global str_bool_dict
        if value in str_bool_dict:
            field_value = str_bool_dict[value]
        else:
            field_value = None
    elif field_type is list or field_type is List or get_origin(field_type) is Annotated and (
            get_args(field_type)[0] is list or get_args(field_type)[0] is List or
            get_origin(get_args(field_type)[0]) is list or get_origin(get_args(field_type)[0]) is List):
        try:
            field_value = eval(value)
            if not isinstance(field_value, List) and all(map(lambda x: isinstance(x, str), field_value)):
                field_value = None
        except SyntaxError:
            field_value = None
    else:
        field_value = None

    return field_value


__all__ = [
    "DocRole",
    "DescriptionRole",
    "parse_field"
]
