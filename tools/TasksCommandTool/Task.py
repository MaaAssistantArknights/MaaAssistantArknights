from __future__ import annotations

import math

from .debug import trace
from .TaskType import AlgorithmType, ActionType, TaskDerivedType

# 存储所有任务
_ALL_TASKS = {}

# TaskPipelineInfo
# see src/MaaCore/Common/AsstTypes.h
_TASK_PIPELINE_INFO_FIELDS = ["sub", "next", "onErrorNext", "exceededNext", "reduceOtherTimes"]
# TaskInfo 中的字段
_TASK_INFO_FIELDS = ["algorithm", "action", "subErrorIgnored", "maxTimes", "specificRect", "preDelay",
                     "postDelay", "retryTimes", "roi", "rectMove", "cache", "specialParams"]
# 继承时需要处理的字段
_TASK_EXTEND_FIELDS = _TASK_PIPELINE_INFO_FIELDS + _TASK_INFO_FIELDS


def _is_template_task_name(name: str) -> bool:
    return name.__contains__('@')


def _is_base_task(task: Task) -> bool:
    return task.base_task is not None


@trace
def _extend_task_dict(base: dict, override: dict | None, prefix: str) -> dict:
    def add_prefix(s: list[str]) -> list[str]:
        return [f"{prefix}@" + x if not x.startswith('#') else f"{prefix}" + x for x in s]

    task_dict = {}
    assert base is not None
    if override is None:
        task_dict = base.copy()
    elif algorithm := override.get("algorithm", None):
        assert base.__contains__("algorithm")
        if algorithm != base["algorithm"]:
            task_dict = {field: base[field] for field in _TASK_EXTEND_FIELDS if field in base}
            # 覆盖任务名
            task_dict["algorithm"] = algorithm

    for field in _TASK_PIPELINE_INFO_FIELDS:
        if field in task_dict:
            task_dict[field] = add_prefix(task_dict[field])
    return task_dict


class Task:
    def __init__(self, name: str, task_dict: dict):
        # 任务名
        self.name = name
        # 任务字典
        self.task_dict = task_dict
        self.derived_type = TaskDerivedType.Raw
        # =============== Begin Task Fields ================
        # 以xxx任务为模板生成任务，详细见下方特殊任务类型中的Base Task
        self.base_task = task_dict.get("baseTask", None)
        # 可选项，表示识别算法的类型
        self.algorithm = AlgorithmType(task_dict.get("algorithm", "MatchTemplate"))
        # 可选项，表示识别到后的动作
        self.action = ActionType(task_dict.get("action", "DoNothing"))
        # 可选项，子任务。会在执行完当前任务后，依次执行每一个子任务
        self.sub_tasks = task_dict.get("sub", None)
        # 可选项，是否忽略子任务的错误。
        self.sub_error_ignored = task_dict.get("subErrorIgnored", False)
        # 可选项，表示执行完当前任务和 sub 任务后，下一个要 execute 的任务
        self.next = task_dict.get("next", None)
        # 可选项，表示该任务最大执行次数
        self.max_times = task_dict.get("maxTimes", math.inf)
        # 可选项，表示达到了最大执行次数后要执行的任务
        self.exceeded_next = task_dict.get("exceededNext", None)
        # 可选项，表示执行出错时，后续要执行的任务
        self.on_error_next = task_dict.get("onErrorNext", None)
        # 可选项，表示识别到后延迟多久才执行 action，单位毫秒
        self.pre_delay = task_dict.get("preDelay", 0)
        # 可选项，表示 action 执行完后延迟多久才去识别 next, 单位毫秒
        self.post_delay = task_dict.get("postDelay", 0)
        # 可选项，表示识别的范围，格式为 [ x, y, width, height ]
        self.roi = task_dict.get("roi", [0, 0, 1280, 720])
        # 可选项，表示该任务是否使用缓存，默认为 true
        self.cache = task_dict.get("cache", True)
        # 可选项，识别后的目标移动，不建议使用该选项。以 1280 * 720 为基准自动缩放
        self.rect_move = task_dict.get("rectMove", [0, 0, 0, 0])
        # 可选项，执行后减少其他任务的执行计数。
        self.reduce_other_times = task_dict.get("reduceOtherTimes", None)
        # 当 action 为 ClickRect 时有效且必选，表示指定的点击位置（范围内随机一点）。
        self.specific_rect = task_dict.get("specificRect", None)
        # 可选项，表示该任务是否使用缓存，默认为 true
        self.special_params = task_dict.get("specialParams", None)
        # =============== Begin MatchTemplate ================
        if self.algorithm == AlgorithmType.MatchTemplate:
            # 可选项，要匹配的图片文件名
            self.template = task_dict.get("template", f"{name}.png")
            # 可选项，图片模板匹配得分的阈值，超过阈值才认为识别到了。
            self.templ_threshold = task_dict.get("templThreshold", 0.8)
            # 可选项，灰度掩码范围。例如将图片不需要识别的部分涂成黑色（灰度值为 0）
            self.mask_range = task_dict.get("maskRange", [1, 255])
        # =============== End MatchTemplate ================
        # =============== Begin OcrDetect ================
        if self.algorithm == AlgorithmType.OcrDetect:
            # 必选项，要识别的文字内容，只要任一匹配上了即认为识别到了
            self.text = task_dict.get("text", None)
            # 可选项，针对常见识别错的文字进行替换（支持正则）
            self.ocr_replace = task_dict.get("ocrReplace", None)
            # 可选项，文字识别是否需要全字匹配（不能多字），默认为 false
            self.full_match = task_dict.get("fullMatch", False)
            # 可选项，要识别的文字内容是否为 ASCII 码字符
            self.is_ascii = task_dict.get("isAscii", False)
            # 可选项，是否不使用检测模型
            self.without_det = task_dict.get("withoutDet", False)
        # =============== End OcrDetect ================
        # =============== Begin Hash ================
        # TODO
        # =============== End Hash ================
        # =============== End Task Fields ================
        _ALL_TASKS[name] = self

    def __str__(self):
        return f"{self.derived_type.value}Task({self.name})"

    def __repr__(self):
        return str(self.__dict__)

    @staticmethod
    @trace
    def get(name, parent=None):
        task = _ALL_TASKS.get(name, None)
        if task is not None:
            if _is_base_task(task):
                return Task._build_base_task(task)
            return task
        elif _is_template_task_name(name):
            return Task._build_template_task(name)
        else:
            raise ValueError(f"Task {name} not found.")

    @staticmethod
    @trace
    def _build_base_task(task: Task) -> Task:
        if type(task) is BaseTask:
            return task
        base_dict = Task.get(task.base_task).task_dict.copy()
        base_dict.update(task.task_dict)
        base_dict.pop("baseTask")
        return BaseTask(task.name, base_dict)

    @staticmethod
    def _build_template_task(name: str) -> Task:
        first, *rest = name.split('@')
        rest = '@'.join(rest)
        base_task = Task.get(rest)
        if _ALL_TASKS.__contains__(name):
            task_dict = _extend_task_dict(base_task.task_dict, _ALL_TASKS[name].task_dict, first)
        else:
            task_dict = _extend_task_dict(base_task.task_dict, None, first)
        return TemplateTask(name, task_dict)


class TemplateTask(Task):

    def __init__(self, name: str, task_dict: dict):
        super().__init__(name, task_dict)
        self.derived_type = TaskDerivedType.Template


class BaseTask(Task):

    def __init__(self, name: str, task_dict: dict):
        super().__init__(name, task_dict)
        self.derived_type = TaskDerivedType.Base


class VirtualTask(Task):
    pass
