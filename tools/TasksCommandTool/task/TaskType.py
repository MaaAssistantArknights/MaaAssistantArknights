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


class TaskStatus(Enum):
    # Enum for task derived types
    Uninitialized = "Uninitialized"
    Raw = "Raw"
    Initialized = "Initialized"
    Interpreted = "Interpreted"