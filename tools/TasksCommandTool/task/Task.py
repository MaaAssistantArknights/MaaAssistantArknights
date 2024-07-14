from __future__ import annotations

from .debug import trace
from .TaskType import AlgorithmType, TaskDerivedType
from .TaskField import TaskFieldEnum
from .TaskExpression import _tokenize, _shunting_yard

# 存储所有任务
_ALL_TASKS = {}

# TaskPipelineInfo 中的字段
# see src/MaaCore/Common/AsstTypes.h
_TASK_PIPELINE_INFO_FIELDS = [TaskFieldEnum.SUB_TASKS, TaskFieldEnum.NEXT, TaskFieldEnum.ON_ERROR_NEXT,
                              TaskFieldEnum.EXCEEDED_NEXT, TaskFieldEnum.REDUCE_OTHER_TIMES]
# TaskInfo 中的字段 继承TaskPipelineInfo
_TASK_INFO_FIELDS = _TASK_PIPELINE_INFO_FIELDS + [TaskFieldEnum.ALGORITHM, TaskFieldEnum.ACTION,
                                                  TaskFieldEnum.SUB_ERROR_IGNORED, TaskFieldEnum.MAX_TIMES,
                                                  TaskFieldEnum.SPECIFIC_RECT, TaskFieldEnum.PRE_DELAY,
                                                  TaskFieldEnum.POST_DELAY, TaskFieldEnum.ROI,
                                                  TaskFieldEnum.RECT_MOVE, TaskFieldEnum.CACHE,
                                                  TaskFieldEnum.SPECIAL_PARAMS]


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
            for field in _TASK_INFO_FIELDS:
                field = field.value
                if field.field_name in base:
                    task_dict[field.field_name] = base[field.field_name]
            # 覆盖任务名
            task_dict["algorithm"] = algorithm

    for field in _TASK_PIPELINE_INFO_FIELDS:
        field = field.value
        if field.field_name in task_dict:
            task_dict[field.field_name] = add_prefix(task_dict[field.field_name])
    return task_dict


class Task:
    def __init__(self, name: str, task_dict: dict):
        # 任务名
        self.name = name
        self.derived_type = TaskDerivedType.Raw
        self.task_dict = task_dict

        self.algorithm = task_dict.get("algorithm", AlgorithmType.MatchTemplate)
        self.base_task = task_dict.get("baseTask", None)

        for field in TaskFieldEnum:
            field = field.value
            if field.valid_for_algorithm is None or field.valid_for_algorithm == self.algorithm:
                field_value = task_dict.get(field.field_name, field.field_default)
                if field.is_valid_with(field_value):
                    setattr(self, field.python_field_name, field_value)
                else:
                    raise ValueError(f"Field {field.field_name} is invalid with value {field_value}")

        if self.algorithm == AlgorithmType.MatchTemplate and self.template is None:
            self.template = f"{self.name}.png"

        _ALL_TASKS[name] = self

    def __str__(self):
        return f"{self.derived_type.value}Task({self.name})"

    def __repr__(self):
        return str(self.__dict__)

    def to_task_dict(self):
        task_dict = self.task_dict.copy()
        for field in TaskFieldEnum:
            field = field.value
            if field.valid_for_algorithm is None or field.valid_for_algorithm == self.algorithm:
                task_dict[field.field_name] = self.__getattribute__(field.python_field_name)
        return task_dict

    def interpret(self):
        for field in _TASK_PIPELINE_INFO_FIELDS:
            field = field.value
            setattr(self, field.field_name, Task.evaluate(getattr(self, field.field_name), self))

    def show(self):
        for field in _TASK_PIPELINE_INFO_FIELDS:
            field = field.value
            if self.__dict__.__contains__(field):
                print(field)
                print(' -> '.join(['   |', *self.__getattribute__(field)]))

    def print(self):
        for key, value in self.__dict__.items():
            if key == "task_dict":
                continue
            print(f"{key}: {value}")

    @staticmethod
    def load_tasks(tasks: dict):
        for name, task_dict in tasks.items():
            Task(name, task_dict)

    @staticmethod
    @trace
    def get(name) -> Task:
        if isinstance(name, Task):
            return name
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
        base_dict = Task.get(task.base_task).to_task_dict().copy()
        base_dict.update(task.to_task_dict())
        base_dict.pop(TaskFieldEnum.BASE_TASK.value.field_name)
        return BaseTask(task.name, base_dict, task)

    @staticmethod
    def _build_template_task(name: str) -> Task:
        first, *rest = name.split('@')
        rest = '@'.join(rest)
        base_task = Task.get(rest)
        if _ALL_TASKS.__contains__(name):
            task_dict = _extend_task_dict(base_task.task_dict, _ALL_TASKS[name].task_dict, first)
            return TemplateTask(name, task_dict, _ALL_TASKS[name])
        else:
            task_dict = _extend_task_dict(base_task.task_dict, None, first)
            return TemplateTask(name, task_dict, None)

    @staticmethod
    def _eval_virtual_task(task_name_context: str | None, virtual_task_type: str, parent_task: Task) -> Task:
        if virtual_task_type != "self" and task_name_context is None:
            raise ValueError(f"Invalid virtual task context: {task_name_context}")
        if virtual_task_type == "self":
            return parent_task
        elif virtual_task_type == "back":
            return Task.get(task_name_context)
        elif virtual_task_type == "next":
            return Task.get(task_name_context).next
        elif virtual_task_type == "sub":
            return Task.get(task_name_context).sub_tasks
        elif virtual_task_type == "on_error_next":
            return Task.get(task_name_context).on_error_next
        elif virtual_task_type == "exceeded_next":
            return Task.get(task_name_context).exceeded_next
        elif virtual_task_type == "reduce_other_times":
            return Task.get(task_name_context).reduce_other_times
        else:
            raise ValueError(f"Invalid virtual task type: {virtual_task_type}")

    @staticmethod
    def evaluate(expression: str, parent_task: Task = None):
        tokens = _tokenize(expression)
        postfix = _shunting_yard(tokens)
        stack = []
        for token in postfix:
            if token == '#u':
                right = stack.pop()
                stack.append(Task._eval_virtual_task(None, right, parent_task))
            elif token == '#':
                right = stack.pop()
                left = stack.pop()
                stack.append(Task._eval_virtual_task(left, right, parent_task))
            elif token == '*':
                right = stack.pop()
                left = stack.pop()
                stack.append([left] * right)
            elif token == '+':
                right = stack.pop()
                left = stack.pop()
                stack.append([left, right])
            elif token == '^':
                # 任务列表差（在前者但不在后者，顺序不变）
                right = stack.pop()
                left = stack.pop()
                assert isinstance(left, list) and isinstance(right, list), f"Invalid expression: {expression}"
                stack.append([x for x in left if x not in right])
            else:
                stack.append(Task.get(token))
        return stack.pop()


class TemplateTask(Task):

    def __init__(self, name: str, task_dict: dict, raw_task: Task | None):
        super().__init__(name, task_dict)
        self.derived_type = TaskDerivedType.Template
        self.raw_task = raw_task


class BaseTask(Task):

    def __init__(self, name: str, task_dict: dict, raw_task: Task):
        super().__init__(name, task_dict)
        self.derived_type = TaskDerivedType.Base
        self.raw_task = raw_task


class VirtualTask(Task):
    pass
