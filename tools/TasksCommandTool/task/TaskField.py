import math
from enum import Enum
from types import UnionType
from typing import Any, Callable, Type, Union, List, get_origin, get_args

from .TaskType import AlgorithmType, ActionType


class TaskField:
    def __init__(self,
                 field_name: str,
                 python_field_name: str,
                 field_type: Union[type, UnionType, List[Type]],
                 field_doc: str,
                 field_default: any = None,
                 is_valid_with: Callable[[Any], bool] = lambda x: True,
                 valid_for_algorithm: AlgorithmType | None = None,
                 ):
        self.field_name = field_name
        self.python_field_name = python_field_name
        self.field_type = field_type
        self.field_doc = field_doc.strip(" ")
        self.field_default = field_default
        self.is_valid_with = lambda x: x is None or self._check_type(x) and is_valid_with(x)
        self.valid_for_algorithm = valid_for_algorithm
        assert self.is_valid_with(field_default)

    def _check_type(self, x: Any) -> bool:
        def _check_type(expected_type: Union[type, UnionType, List[Type]], value) -> bool:
            origin = get_origin(expected_type)
            args = get_args(expected_type)

            if origin is list and args:
                if not isinstance(x, list):
                    return False
                return all(isinstance(v, args[0]) for v in x)
            if origin is UnionType:
                return any(_check_type(t, value) for t in args)

            # if hasattr(expected_type, '__origin__') and expected_type.__origin__ is Union:
            #     expected_type = tuple(expected_type.__args__)

            return isinstance(x, expected_type)

        return _check_type(self.field_type, x)


class TaskFieldEnum(Enum):
    BASE_TASK = TaskField(
        "baseTask",
        "base_task",
        str,
        "以xxx任务为模板生成任务，详细见下方特殊任务类型中的Base Task",
    )
    ALGORITHM = TaskField(
        "algorithm",
        "algorithm",
        str | AlgorithmType,
        "可选项，表示识别算法的类型",
        "MatchTemplate",
        lambda x: x in AlgorithmType,
    )
    ACTION = TaskField(
        "action",
        "action",
        str | ActionType,
        "可选项，表示识别到后的动作",
        "DoNothing",
        lambda x: x in ActionType,
    )
    SUB_TASKS = TaskField(
        "sub",
        "sub_tasks",
        list[str],
        "可选项，子任务。会在执行完当前任务后，依次执行每一个子任务",
    )
    SUB_ERROR_IGNORED = TaskField(
        "subErrorIgnored",
        "sub_error_ignored",
        bool,
        "可选项，是否忽略子任务的错误。",
        False,
    )
    NEXT = TaskField(
        "next",
        "next",
        list[str],
        "可选项，表示执行完当前任务和 sub 任务后，下一个要 execute 的任务",
    )
    MAX_TIMES = TaskField(
        "maxTimes",
        "max_times",
        int | float,
        "可选项，表示该任务最大执行次数",
        math.inf,
        lambda x: x == math.inf or (isinstance(x, int) and x >= 0),
    )
    EXCEEDED_NEXT = TaskField(
        "exceededNext",
        "exceeded_next",
        list[str],
        "可选项，表示达到了最大执行次数后要执行的任务",
    )
    ON_ERROR_NEXT = TaskField(
        "onErrorNext",
        "on_error_next",
        list[str],
        "可选项，表示执行出错时，后续要执行的任务",
    )
    PRE_DELAY = TaskField(
        "preDelay",
        "pre_delay",
        int,
        "可选项，表示识别到后延迟多久才执行 action，单位毫秒",
        0,
        lambda x: x >= 0,
    )
    POST_DELAY = TaskField(
        "postDelay",
        "post_delay",
        int,
        "可选项，表示执行完 action 后延迟多久才执行下一个任务，单位毫秒",
        0,
        lambda x: x >= 0,
    )
    ROI = TaskField(
        "roi",
        "roi",
        list[int],
        "可选项，表示识别的区域",
        [0, 0, 1280, 720],
        lambda x: len(x) == 4,
    )
    CACHE = TaskField(
        "cache",
        "cache",
        bool,
        "可选项，表示该任务是否使用缓存，默认为 true",
        False,
    )
    RECT_MOVE = TaskField(
        "rectMove",
        "rect_move",
        list[int],
        "可选项，表示识别到后移动的区域",
        None,
        lambda x: len(x) == 4,
    )
    REDUCE_OTHER_TIMES = TaskField(
        "reduceOtherTimes",
        "reduce_other_times",
        list[str],
        "可选项，表示减少其他任务的执行次数",
    )
    SPECIFIC_RECT = TaskField(
        "specificRect",
        "specific_rect",
        list[int],
        "可选项，表示特殊区域",
        None,
        lambda x: len(x) == 4,
    )
    SPECIAL_PARAMS = TaskField(
        "specialParams",
        "special_params",
        list,
        "可选项，表示特殊参数",
    )
    TEMPLATE = TaskField(
        "template",
        "template",
        str | list[str],
        "可选项，表示模板路径",
        valid_for_algorithm=AlgorithmType.MatchTemplate,
    )
    TEMPL_THRESHOLD = TaskField(
        "templThreshold",
        "templ_threshold",
        float | list[float],
        "可选项，表示模板匹配的阈值",
        0.8,
        lambda x: 0 <= x <= 1 if isinstance(x, float) else all(0 <= i <= 1 for i in x),
        valid_for_algorithm=AlgorithmType.MatchTemplate,
    )
    MASK_RANGE = TaskField(
        "maskRange",
        "mask_range",
        list[int],
        "可选项，表示模板匹配的掩码范围",
        [1, 255],
        lambda x: 0 <= x[0] <= x[1] <= 255,
        valid_for_algorithm=AlgorithmType.MatchTemplate,
    )
    TEXT = TaskField(
        "text",
        "text",
        list[str],
        "可选项，表示要识别的文字内容",
        valid_for_algorithm=AlgorithmType.OcrDetect,
    )
    FULL_MATCH = TaskField(
        "fullMatch",
        "full_match",
        bool,
        "可选项，表示是否全字匹配",
        False,
        lambda x: isinstance(x, bool),
        valid_for_algorithm=AlgorithmType.OcrDetect,
    )
    IS_ASCII = TaskField(
        "isAscii",
        "is_ascii",
        bool,
        "可选项，表示是否为 ASCII 码字符",
        False,
        valid_for_algorithm=AlgorithmType.OcrDetect,
    )
    WITHOUT_DET = TaskField(
        "withoutDet",
        "without_det",
        bool,
        "可选项，表示是否不使用检测模型",
        False,
        valid_for_algorithm=AlgorithmType.OcrDetect,
    )
    OCR_REPLACE = TaskField(
        "ocrReplace",
        "ocr_replace",
        list,
        "可选项，表示是否替换识别错误的文字",
        None,
        lambda x: all(isinstance(i, list) and len(i) == 2 for i in x),
        valid_for_algorithm=AlgorithmType.OcrDetect,
    )
    REPLACE_FULL = TaskField(
        "replaceFull",
        "replace_full",
        bool,
        "可选项，表示是否替换全字",
        False,
        lambda x: isinstance(x, bool),
        valid_for_algorithm=AlgorithmType.OcrDetect,
    )
    REPLACE_MAP = TaskField(
        "replaceMap",
        "replace_map",
        list,
        "可选项，表示替换的字典",
        None,
        lambda x: all(isinstance(i, list) and len(i) == 2 for i in x),
        valid_for_algorithm=AlgorithmType.OcrDetect,
    )
