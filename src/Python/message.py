from enum import Enum, unique, auto


@unique
class Message(Enum):
    """
    回调消息

    请参考 https://github.com/MistEO/MeoAssistantArknights/wiki/%E5%9B%9E%E8%B0%83%E6%B6%88%E6%81%AF
    """
    InternalError = 0

    InitFailed = auto()

    ConnectionError = auto()

    AllTasksCompleted = auto()

    TaskChainError = 10000

    TaskChainStart = auto()

    TaskChainCompleted = auto()

    TaskChainExtraInfo = auto()

    SubTaskError = 20000

    SubTaskStart = auto()

    SubTaskCompleted = auto()
    
    SubTaskExtraInfo = auto()
