from enum import Enum


class AlgorithmType(Enum):
    # Enum for algorithm types
    JustReturn = "JustReturn"
    MatchTemplate = "MatchTemplate"
    OcrDetect = "OcrDetect"
    Hash = "Hash"

    def __eq__(self, other):
        return self.value == other


class ActionType(Enum):
    # Enum for actions
    ClickSelf = "ClickSelf"
    ClickRand = "ClickRand"
    ClickRect = "ClickRect"
    DoNothing = "DoNothing"
    Stop = "Stop"
    Swipe = "Swipe"

    def __eq__(self, other):
        return self.value == other


class TaskDerivedType(Enum):
    # Enum for task derived types
    Raw = "Raw"
    Base = "Base"
    Template = "Template"
    Virtual = "Virtual"
    Interpreted = "Interpreted"
