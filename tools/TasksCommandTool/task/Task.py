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

_VIRTUAL_TASK_TYPES = ["self", "back", "next", "sub", "on_error_next", "exceeded_next", "reduce_other_times"]


def _is_template_task_name(name: str) -> bool:
    return name.__contains__('@')


def _is_base_task(task: Task) -> bool:
    return task.base_task is not None


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
    """@DynamicAttrs"""

    def __init__(self, name: str, task_dict: dict):
        # 任务名
        self.name = name

        self.derived_type = TaskDerivedType.Raw
        self._task_dict = task_dict

        self.algorithm = task_dict.get("algorithm", AlgorithmType.MatchTemplate)
        self.base_task = task_dict.get("baseTask", None)

        for field in TaskFieldEnum:
            field = field.value
            if field.valid_for_algorithm is None or field.valid_for_algorithm == self.algorithm:
                field_value = task_dict.get(field.field_name, field.field_default)
                try:
                    field_valid = field.is_valid_with(field_value)
                except Exception as e:
                    raise ValueError(f"Error checking field {field.field_name}: {e}")
                if field_valid:
                    setattr(self, field.python_field_name, field_value)
                else:
                    raise ValueError(f"Field {field.field_name} is invalid with value {field_value}")

        if self.algorithm == AlgorithmType.MatchTemplate and self.template is None:
            self.template = f"{self.name}.png"

        _ALL_TASKS[name] = self

    def __str__(self):
        return f"{self.derived_type.value}Task({self.name})"

    def __repr__(self):
        return str(self)

    def to_task_dict(self):
        task_dict = self._task_dict.copy()
        task_dict["name"] = self.name
        for field in TaskFieldEnum:
            field = field.value
            if field.valid_for_algorithm is None or field.valid_for_algorithm == self.algorithm:
                task_dict[field.field_name] = getattr(self, field.python_field_name)
        return task_dict

    def interpret(self) -> InterpretedTask:
        if isinstance(self, InterpretedTask):
            return self
        interpreted_task = InterpretedTask(self.name, self)
        for field in _TASK_PIPELINE_INFO_FIELDS:
            field = field.value
            task_list = getattr(interpreted_task, field.python_field_name)
            if task_list is not None:
                interpreted_task_list = []
                for task in task_list:
                    interpreted_task_list.extend(Task.evaluate(task, interpreted_task))
                for i, task in enumerate(interpreted_task_list):
                    if isinstance(task, Task):
                        interpreted_task_list[i] = task.name
                # 去重
                interpreted_task_set = set()
                interpreted_task_without_duplicate = []
                for task in interpreted_task_list:
                    if task not in interpreted_task_set:
                        interpreted_task_set.add(task)
                        interpreted_task_without_duplicate.append(task)
                setattr(interpreted_task, field.python_field_name, interpreted_task_without_duplicate)
        return interpreted_task

    def show(self):
        for field in _TASK_PIPELINE_INFO_FIELDS:
            field = field.value
            if self.to_task_dict():
                print(field)
                print(' -> '.join(['   |', *self.__getattribute__(field)]))

    def print(self):
        for key, value in self.to_task_dict():
            print(f"{key}: {value}")

    @staticmethod
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
            task_dict = _extend_task_dict(base_task._task_dict, _ALL_TASKS[name]._task_dict, first)
            return TemplateTask(name, task_dict, _ALL_TASKS[name])
        else:
            task_dict = _extend_task_dict(base_task._task_dict, None, first)
            return TemplateTask(name, task_dict, None)

    @staticmethod
    @trace
    def _eval_virtual_task(task_name_context: str | None, virtual_task_type: str, parent_task: Task):
        if virtual_task_type != "self" and task_name_context is None:
            return None
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
    @trace
    def evaluate(expression: str, parent_task: Task = None):
        assert expression is not None, "Expression cannot be None."

        def _to_list(x):
            return [x] if not isinstance(x, list) else x

        tokens = _tokenize(expression)
        postfix = _shunting_yard(tokens)
        stack = []
        for token in postfix:
            if token == '#u':
                right = stack.pop()
                task = Task._eval_virtual_task(None, right, parent_task)
                if task is not None:
                    stack.append(task)
            elif token == '#':
                right = stack.pop()
                left = stack.pop()
                task = Task._eval_virtual_task(left, right, parent_task)
                if task is not None:
                    stack.append(task)
            elif token == '*':
                right = stack.pop()
                left = stack.pop()
                stack.append(left * right)
            elif token == '+':
                right = _to_list(stack.pop())
                left = _to_list(stack.pop())
                stack.append(left + right)
            elif token == '^':
                # 任务列表差（在前者但不在后者，顺序不变）
                right = _to_list(stack.pop())
                left = _to_list(stack.pop())
                stack.append([x for x in left if x not in right])
            else:
                if isinstance(token, int):
                    stack.append(token)
                elif token in _VIRTUAL_TASK_TYPES:
                    stack.append(token)
                else:
                    task = Task.get(token)
                    stack.append(task)
        if len(stack) == 0:
            return []
        elif len(stack) == 1:
            return _to_list(stack.pop())
        else:
            raise ValueError(f"Invalid expression: {expression}")


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


class InterpretedTask(Task):

    def __init__(self, name: str, raw_task: Task):
        super().__init__(name, raw_task.to_task_dict())
        self.derived_type = TaskDerivedType.Interpreted
        self.raw_task = raw_task
