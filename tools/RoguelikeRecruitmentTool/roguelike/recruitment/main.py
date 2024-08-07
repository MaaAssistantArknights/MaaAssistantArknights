from collections import Counter
from contextlib import contextmanager
from contextvars import ContextVar
from copy import deepcopy
from os import system
from pathlib import Path
from pydantic import (
    BaseModel,
    Field,
    Strict,
    StrictBool,
    StrictInt,
    StringConstraints,
    ValidationInfo,
    conlist,
    model_serializer,
    model_validator
)
from typing import Any, ClassVar, ForwardRef, Iterator
from typing_extensions import Annotated, Self

from roguelike.config import Theme

# ================================================================================
# Context
# ================================================================================
_context_var: ContextVar[dict[str, Any]] = ContextVar("_context_var", default=dict())


@contextmanager
def context(value: dict[str, Any]) -> Iterator[None]:
    token = _context_var.set(value)
    try:
        yield
    finally:
        _context_var.reset(token)


# ================================================================================
# Types
# ================================================================================
# StrictStr is different from pydantic.types.StrictStr by "strip_whitespace=True"
StrictStr = Annotated[str, StringConstraints(strip_whitespace=True, strict=True)]

StrictNonEmptyStr = Annotated[str, StringConstraints(strip_whitespace=True, strict=True, min_length=1)]


# ================================================================================
# Class RecruitPriorityOffset
# ================================================================================
class RecruitPriorityOffset(BaseModel, extra="forbid"):
    groups: conlist(item_type=StrictNonEmptyStr, min_length=1)
    is_less: StrictBool = False
    threshold: StrictInt = 0
    offset: StrictInt = 0
    doc: StrictStr = ""

    @model_validator(mode="after")
    def sanity_check(self, info: ValidationInfo) -> Self:
        if info.context and info.context.get("skip_validation", False):
            return self
        counts = Counter(self.groups)
        if any(count >= 2 for count in counts.values()):
            raise AssertionError("duplicate operators")
        if self.offset == 0:
            raise AssertionError("offset for cannot be zero")
        return self

    def __hash__(self):
        return hash(self.model_dump_json())


# ================================================================================
# Class CollectionPriorityOffset
# ================================================================================
class CollectionPriorityOffset(BaseModel, extra="forbid"):
    collection: StrictNonEmptyStr
    offset: StrictInt = 0


# ================================================================================
# Class Oper
# ================================================================================
OperClass = ForwardRef("Oper")


class Oper(BaseModel, extra="forbid"):
    name: StrictNonEmptyStr
    doc: StrictStr = ""
    skill: StrictInt = 0
    skill_usage: Annotated[StrictInt, Field(ge=0, le=3, validate_default=True)] = 1
    skill_times: StrictInt = 1
    alternate_skill: StrictInt = 0
    alternate_skill_usage: Annotated[StrictInt, Field(ge=0, le=3, validate_default=True)] = 1
    alternate_skill_times: StrictInt = 1
    is_key: StrictBool = False
    is_start: StrictBool = False
    is_alternate: StrictBool = False
    auto_retreat: StrictInt = 0
    recruit_priority: StrictInt = 0  # Annotated[StrictInt, Field(ge=0, le=1000, validate_default=True)]
    promote_priority: StrictInt = 0  # Annotated[StrictInt, Field(ge=0, le=1000, validate_default=True)]
    recruit_priority_when_team_full: StrictInt | None = None  # Annotated[StrictInt, Field(ge=0, le=1000)]
    promote_priority_when_team_full: StrictInt | None = None  # Annotated[StrictInt, Field(ge=0, le=1000)]
    recruit_priority_offsets: conlist(
        item_type=Annotated[RecruitPriorityOffset, Field(strict=True, validate_default=True)]) = list()
    collection_priority_offsets: conlist(
        item_type=Annotated[CollectionPriorityOffset, Field(strict=True, validate_default=True)]) = list()

    _inherited_attributes: ClassVar[set[str]] = {
        "skill",
        "alternate_skill",
        "skill_usage",
        "skill_times",
        "alternate_skill_usage",
        "alternate_skill_times",
        "is_key",
        "is_start",
        "is_alternate",
        "auto_retreat",
        "recruit_priority",
        "promote_priority"
    }

    def __init__(self, /, **data: Any) -> None:
        oper_info_cache = _context_var.get()["oper_info_cache"]
        oper_name = data.get("name")
        cache = oper_info_cache.get(oper_name, dict())
        for key in Oper._inherited_attributes:
            if key not in data and key in cache:
                data[key] = cache[key]
        if "recruit_priority_when_team_full" not in data:
            data["recruit_priority_when_team_full"] = data.get("recruit_priority", 0) - 100
        if "promote_priority_when_team_full" not in data:
            data["promote_priority_when_team_full"] = data.get("promote_priority", 0) + 300
        data["recruit_priority_offsets"] = (cache.get("recruit_priority_offsets", list())
                                            + data.get("recruit_priority_offsets", list()))
        if oper_name:
            oper_info_cache[oper_name] = deepcopy(data)
        super().__init__(**data)

    @model_validator(mode="after")
    def sanity_check(self, info: ValidationInfo) -> Self:
        if info.context and info.context.get("skip_validation", False):
            return self
        counts = Counter(self.recruit_priority_offsets)
        if any(count >= 2 for count in counts.values()):
            raise AssertionError("duplicate priority offsets")
        return self

    @model_serializer(mode="wrap")
    def serialize_model(self, handler, info):
        oper_info_cache = _context_var.get()["oper_info_cache"]
        tmp = deepcopy(self)
        # ————————————————————————————————————————————————————————————————
        if tmp.recruit_priority_when_team_full == tmp.recruit_priority - 100:
            tmp.recruit_priority_when_team_full = None
        if tmp.promote_priority_when_team_full == tmp.promote_priority + 300:
            tmp.promote_priority_when_team_full = None
        if tmp.name in oper_info_cache:
            cache = oper_info_cache[tmp.name]
            for key in Oper._inherited_attributes:
                if getattr(tmp, key) == getattr(cache, key):
                    setattr(tmp, key, Oper.model_fields[key].get_default())
            cache_len: int = len(getattr(cache, "recruit_priority_offsets"))
            assert len(tmp.recruit_priority_offsets) >= cache_len
            assert tmp.recruit_priority_offsets[: cache_len] == getattr(cache, "recruit_priority_offsets")
            tmp.recruit_priority_offsets = tmp.recruit_priority_offsets[cache_len:]
        # ————————————————————————————————————————————————————————————————
        oper_info_cache[self.name] = deepcopy(self)
        return handler(tmp, info)

    def __eq__(self, other: OperClass) -> bool:
        return self.name == other.name and self.skill == other.skill

    def __hash__(self):
        return hash((self.name, self.skill))

    @classmethod
    def json2oper(cls, json_str: str) -> OperClass:
        with context({"oper_info_cache": dict()}):
            config = cls.model_validate_json(json_str, context={"skip_validation": True})
        return config

    @classmethod
    def oper2json(cls, oper: OperClass) -> str:
        with context({"oper_info_cache": dict()}):
            json_str = oper.model_dump_json(indent=4, exclude_defaults=True)
        return json_str


# ================================================================================
# Class Group
# ================================================================================
GroupClass = ForwardRef("Group")


class Group(BaseModel, extra="forbid"):
    name: StrictNonEmptyStr
    doc: StrictStr = ""
    opers: conlist(item_type=Annotated[Oper, Strict()], min_length=1)

    @model_validator(mode="after")
    def sanity_check(self, info: ValidationInfo) -> Self:
        if info.context and info.context.get("skip_validation", False):
            return self
        counts = Counter(self.opers)
        if any(count >= 2 for count in counts.values()):
            raise AssertionError("duplicate operators")
        return self

    def __eq__(self, other: GroupClass) -> bool:
        return self.name == other.name

    def __hash__(self):
        return hash(self.name)


# ================================================================================
# Class TeamCompleteCondition
# ================================================================================
class TeamCompleteCondition(BaseModel, extra="forbid"):
    groups: conlist(item_type=StrictNonEmptyStr, min_length=1)
    threshold: Annotated[StrictInt, Field(ge=1)]

    def __hash__(self):
        return hash(self.model_dump_json())


# ================================================================================
# Class Configuration
# ================================================================================
ConfigurationClass = ForwardRef("Configuration")


class Configuration(BaseModel, extra="forbid"):
    theme: Theme
    priority: conlist(item_type=Annotated[Group, Strict()], min_length=1)
    team_complete_condition: conlist(item_type=Annotated[TeamCompleteCondition, Strict()])

    @model_validator(mode="after")
    def sanity_check(self, info: ValidationInfo) -> Self:
        if info.context and info.context.get("skip_validation", False):
            return self
        counts = Counter(self.priority)
        if any(count >= 2 for count in counts.values()):
            raise AssertionError("duplicate recruitment groups")
        # ————————————————————————————————————————————————————————————————
        counts = Counter(self.team_complete_condition)
        if any(count >= 2 for count in counts.values()):
            raise AssertionError("duplicate team complete conditions")
        # ————————————————————————————————————————————————————————————————
        group_name_set: set[str] = set(g.name for g in self.priority)
        for group in self.priority:
            for oper in group.opers:
                for offset in oper.recruit_priority_offsets:
                    if set(offset.groups) - group_name_set:
                        raise AssertionError(
                            f"unknown group names {set(offset.groups) - group_name_set} "
                            f"in recruit priority offset for operator {oper.name} in group {group.name}")
        # ————————————————————————————————————————————————————————————————
        for item in self.team_complete_condition:
            if set(item.groups) - group_name_set:
                raise AssertionError(
                    f"unknown group names {set(item.groups) - group_name_set} in team_complete_condition")
        # ————————————————————————————————————————————————————————————————
        return self

    @classmethod
    def json2config(cls, json_str: str) -> ConfigurationClass:
        with context({"oper_info_cache": dict()}):
            config = cls.model_validate_json(json_str, context={"skip_validation": True})
        return config

    @classmethod
    def config2json(cls, config: ConfigurationClass) -> str:
        with context({"oper_info_cache": dict()}):
            json_str = config.model_dump_json(indent=4, exclude_defaults=True)
        return json_str


# ================================================================================
# new_oper & new_group
# ================================================================================
def new_recruit_priority_offset(operator_name: str = "unnamed operator") -> RecruitPriorityOffset:
    return RecruitPriorityOffset(groups=[operator_name], offset=100)


def new_collection_priority_offset(collection_name: str = "unnamed collectible") -> CollectionPriorityOffset:
    return CollectionPriorityOffset(collection=collection_name, offset=100)


def new_oper(name: str = "unnamed operator") -> Oper:
    with context({"oper_info_cache": dict()}):
        oper = Oper(name=name)
    return oper


def new_group(name: str = "unnamed group") -> Group:
    return Group(name=name, opers=[new_oper()])
