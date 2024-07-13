import math
from enum import Enum
from typing import Any, Callable

from task.TaskType import AlgorithmType, ActionType


class TaskField:
    def __init__(self,
                 field_name: str,
                 python_field_name: str,
                 field_type: Any,
                 field_doc: str,
                 field_default: any = None,
                 is_valid_with: Callable[[Any], bool] = lambda x: True,
                 valid_for_algorithm: AlgorithmType | None = None,
                 ):
        self.field_name = field_name
        self.python_field_name = python_field_name
        self.field_type = field_type
        self.field_doc = field_doc.strip(" ")
        assert field_default is None or isinstance(field_default, field_type)
        self.field_default = field_default
        self.is_valid_with = is_valid_with
        self.valid_for_algorithm = valid_for_algorithm


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
        None,
        lambda x: isinstance(x, AlgorithmType) or isinstance(x, str) and x in AlgorithmType.__members__,
    )
    ACTION = TaskField(
        "action",
        "action",
        str | ActionType,
        "可选项，表示识别到后的动作",
        None,
        lambda x: isinstance(x, ActionType) or isinstance(x, str) and x in ActionType.__members__,
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
        lambda x: x == math.inf or (isinstance(x, int) and x > 0),
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
        lambda x: len(x) == 4 and all(isinstance(i, int) for i in x),
    )
    RECT_MOVE = TaskField(
        "rectMove",
        "rect_move",
        list[int],
        "可选项，表示识别到后移动的区域",
        None,
        lambda x: len(x) == 4 and all(isinstance(i, int) for i in x),
    )
    CACHE = TaskField(
        "cache",
        "cache",
        bool,
        "可选项，表示该任务是否使用缓存，默认为 true",
        True,
    )
    SPECIAL_PARAMS = TaskField(
        "specialParams",
        "special_params",
        list[int],
        "可选项，表示特殊参数",
    )
    TEMPLATE = TaskField(
        "template",
        "template",
        str,
        "可选项，表示模板路径",
        valid_for_algorithm=AlgorithmType.MatchTemplate,
    )
    TEMPL_THRESHOLD = TaskField(
        "templThreshold",
        "templ_threshold",
        float,
        "可选项，表示模板匹配的阈值",
        0.8,
        lambda x: 0 <= x <= 1,
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
        list[list[str, str]],
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
        dict[str, str],
        "可选项，表示替换的字典",
        valid_for_algorithm=AlgorithmType.OcrDetect,
    )
