from enum import Enum, auto


class AlgorithmType(Enum):
    # Enum for algorithm types
    JustReturn = "JustReturn"
    MatchTemplate = "MatchTemplate"
    OcrDetect = "OcrDetect"
    Hash = "Hash"


class ActionType(Enum):
    # Enum for actions
    ClickSelf = "ClickSelf"
    ClickRand = "ClickRand"
    ClickRect = "ClickRect"
    DoNothing = "DoNothing"
    Stop = "Stop"
    Swipe = "Swipe"


class TaskDerivedType(Enum):
    # Enum for task derived types
    Raw = "Raw"
    Base = "Base"
    Template = "Template"
    Virtual = "Virtual"
