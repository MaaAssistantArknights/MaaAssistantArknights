from TaskType import TaskDerivedType, AlgorithmType, ActionType




class TaskPipelineInfo:
    name: str
    next: [str]
    sub: [str]
    on_error_next: [str]
    exceeded_next: [str]
    reduce_other_times: [str]


class TaskDerivedInfo(TaskPipelineInfo):
    type: TaskDerivedType = TaskDerivedType.Raw
    base: str = None
    prefix: str = None


class TaskInfo(TaskPipelineInfo):
    algorithm: AlgorithmType
    action: ActionType
    sub_error_ignored: bool
    max_times: int
    specific_rect: Rect
    pre_delay: int
    post_delay: int
    retry_times: int
    roi: Rect
    rect_move: Rect
    cache: bool
    special_params: [int]


class OcrTaskInfo(TaskInfo):
    text: [str]
    full_match: bool
    is_ascii: bool
    without_det: bool
    replace_full: bool
    replace_map: {str: str}


class MatchTemplateTaskInfo(TaskInfo):
    template: str
    templ_threshold: float
    mask_range: [int]


class HashTaskInfo(TaskInfo):
    pass
