from __future__ import annotations

import json


# ======================= constants begin =========================
class Direction:
    UP = "上"
    DOWN = "下"
    RIGHT = "右"
    LEFT = "左"


class ActionType:
    Deploy = "部署"
    Skill = "技能"
    Retreat = "撤退"
    SpeedUp = "二倍速"
    BulletTime = "子弹时间"
    SkillUsage = "技能用法"
    SkillDaemon = "摆完挂机"
    Output = "打印"


class SkillUsageType:
    NotAutoUse = 0  # 0 - 不自动使用 默认
    UseWhenOk = 1  # 1 - 好了就用，有多少次用多少次（例如干员 棘刺 3 技能、桃金娘 1 技能等）
    UseWhenOkOnce = 2  # 2 - 好了就用，仅使用一次（例如干员 山 2 技能）
    # AutoUse = 3  # 3 - 自动判断使用时机（画饼.jpg）


# ========================= constants end =============================

class Document:
    def __init__(self, title: str, details: str, title_color: str = "dark", details_color: str = "dark"):
        self.details_color = details_color
        self.details = details
        self.title_color = title_color
        self.title = title

    def to_dict(self) -> dict:
        return {
            "title": self.title,
            "title_color": self.title_color,
            "details": self.details,
            "details_color": self.details_color
        }


class Requirements:
    def __init__(self, elite: int, level: int, skill_level: int, module: int, potentiality: int):
        self._potentiality = potentiality
        self._module = module
        self._skill_level = skill_level
        self._level = level
        self._elite = elite

    def to_dict(self):
        return {
            "elite": self._elite,
            "level": self._level,
            "skill_level": self._skill_level,
            "module": self._module,
            "potentiality": self._potentiality
        }


class Action:
    def __init__(self, action_type: str, name: str = None, location: tuple = None, direction: str = "无", kills: int = 0,
                 skill_usage: int = None, pre_delay: int = 0, rear_delay: int = 0, cost_changes: int = 0,
                 timeout: int = 999999999, doc: str = None, doc_color: str = None):
        self._doc_color = doc_color
        self._doc = doc
        # self._timeout = timeout
        self._cost_changes = cost_changes
        self._rear_delay = rear_delay
        self._pre_delay = pre_delay
        self._skill_usage = skill_usage
        self._kills = kills
        self._direction = direction
        self._name = name
        self._location = location
        self._action_type = action_type

    def add_haisi_doc(self):
        if self._doc is not None:
            return
        if self._action_type in [ActionType.SpeedUp, ActionType.BulletTime, ActionType.SkillDaemon, ActionType.Output]:
            haisi_doc_list = [self._action_type, "执行"]
        elif self._action_type in [ActionType.Deploy, ActionType.Skill, ActionType.Retreat]:
            haisi_doc_list = [self._name]
            if self._kills != 0:
                haisi_doc_list.append("等待")
                haisi_doc_list.append(str(self._kills) + "杀")
            if self._pre_delay != 0:
                haisi_doc_list.append("等待")
                haisi_doc_list.append(str(self._pre_delay) + "秒")
            if self._rear_delay != 0:
                haisi_doc_list.append("延时")
                haisi_doc_list.append(str(self._rear_delay) + "秒")
            if self._cost_changes != 0:
                haisi_doc_list.append("等待")
                haisi_doc_list.append("费用")
                haisi_doc_list.append("变化")
                haisi_doc_list.append(str(self._cost_changes))
            haisi_doc_list.append(self._action_type)
        else:
            return
        self._doc = "，".join(list(filter(None, haisi_doc_list)))
        self._doc_color = "orange"

    def to_dict(self) -> dict:
        res = {
            "type": self._action_type,
        }
        if self._kills != 0:
            res["kills"] = self._kills
        if self._pre_delay != 0:
            res["pre_delay"] = self._pre_delay
        if self._rear_delay != 0:
            res["rear_delay"] = self._rear_delay
        if self._cost_changes != 0:
            res["cost_changes"] = self._cost_changes
        # 保留字段，暂未实现。
        # if self._timeout != 0:
        #     res["timeout"] = self._timeout
        if self._action_type == ActionType.Deploy or self._action_type == ActionType.Skill \
                or self._action_type == ActionType.Retreat:
            res["name"] = self._name

        if self._action_type == ActionType.Deploy:
            res["location"] = self._location
            res["direction"] = self._direction
        if self._doc is not None:
            res["doc"] = self._doc
        if self._doc_color is not None:
            res["doc_color"] = self._doc_color
        return res


class OperatorOrGroup:
    def __init__(self, name: str, battle: Battle):
        self._battle = battle
        self.name = name
        self._pre_delay = 0
        self._rear_delay = 0
        self._wait_kills = 0
        self._cost_changes = 0

    def pre_delay(self, times: int) -> OperatorOrGroup:
        self._pre_delay = times
        return self

    def rear_delay(self, times: int) -> OperatorOrGroup:
        self._rear_delay = times
        return self

    def cost_changes(self, costs: int) -> OperatorOrGroup:
        self._cost_changes = costs
        return self

    def wait_kills(self, kills: int) -> OperatorOrGroup:
        self._wait_kills = kills
        return self

    def deploy(self, location: tuple, direction: str):
        self._battle.add_action(self._add_conditions(
            Action(ActionType.Deploy, self.name, location, direction)))

    def use_skill(self):
        self._battle.add_action(self._add_conditions(
            Action(ActionType.Skill, self.name)))

    def retreat(self):
        self._battle.add_action(self._add_conditions(
            Action(ActionType.Retreat, self.name)))

    def _add_conditions(self, action: Action) -> Action:
        if self._pre_delay != 0:
            action._pre_delay = self._pre_delay
        if self._rear_delay != 0:
            action._rear_delay = self._rear_delay
        if self._wait_kills != 0:
            action._kills = self._wait_kills
        if self._cost_changes != 0:
            action._cost_changes = self._cost_changes
        self._clear_conditions()
        return action

    def _clear_conditions(self):
        self._pre_delay = 0
        self._rear_delay = 0
        self._wait_kills = 0
        self._cost_changes = 0

    def to_dict(self) -> dict:
        # 此处不应该被执行到
        raise Exception("请使用子类方法")


class Group(OperatorOrGroup):
    def __init__(self, name: str, battle: Battle, operators: tuple):
        """
        请使用 Battle.create_group()
        :param name:
        :param battle:
        :param operators:
        """
        super().__init__(name, battle)
        self._operators = operators

    def to_dict(self) -> dict:
        return {
            "name": self.name,
            "opers": [operator.to_dict() for operator in self._operators]
        }


class Operator(OperatorOrGroup):
    def __init__(self, name: str, skill: int, skill_usage: int = SkillUsageType.NotAutoUse, battle: Battle = None,
                 requirements: Requirements = None):
        """
        仅当要创建群组时才应该被调用
        正常使用请用 Battle.create_operator()
        :param name:
        :param battle:
        :param skill:
        :param skill_usage:
        :param requirements:
        """
        super().__init__(name, battle)
        self._requirements = requirements
        self._skill_usage = skill_usage
        self._skill = skill
        self._character_name_list = None

    def check_operator_name(self, auto_complete: bool = True) -> bool:
        if self._character_name_list is None:
            import os
            if os.path.exists('./characters/character_name_list.json'):
                with open('./characters/character_name_list.json', 'r', encoding='utf-8') as f:
                    self._character_name_list = json.load(f)
            else:
                raise Exception("无法进行名称检查，请检查名称文件是否存在")
        if self.name in self._character_name_list:
            return True
        if auto_complete:
            possible_character_name = None
            for character_name in self._character_name_list:
                if self.name in character_name:
                    if possible_character_name is None:
                        possible_character_name = character_name
                    else:
                        raise Exception("匹配到多个名字，无法自动补全，请手动补全")
            if possible_character_name is not None:
                self.name = possible_character_name
                return True
            else:
                return False
        else:
            return False

    def skill_usage_change(self, change_to: int):
        """
        改变技能使用的类型
        由于需要兼容群组类型，有时此方法可能无法通过代码提示调用，手动调用即可
        :param change_to: 需要调整到的 SkillUsageType
        :return:
        """
        self._battle.add_action(self._add_conditions(
            Action(ActionType.SkillUsage, skill_usage=change_to)))

    def to_dict(self) -> dict:
        res = {
            "name": self.name,
            "skill": self._skill,
            "skill_usage": self._skill_usage,
        }
        if self._skill_usage != SkillUsageType.NotAutoUse:
            res["skill_usage"] = self._skill_usage
        if self._requirements is not None:
            res["requirements"] = self._requirements.to_dict()
        return res


class Battle:
    def __init__(self, stage_name: str, version="v4.0", doc: Document = None):
        self._doc = doc
        self._stage_name = stage_name
        self._version = version
        self._operators = []
        self._groups = []
        self._actions = []
        self._wait_times = 0
        self._wait_kills = 0
        self._speedup = False
        self._last_file_name = None
        # 海嗣语 我，海嗣，回归大群，打钱
        self._enable_haisi_doc = False

    def enable_haisi_doc(self) -> Battle:
        self._enable_haisi_doc = True
        self._doc.details += "\n"
        self._doc.details += "回归，大群，加入，光荣，进化"
        return self

    def create_operator(self, name: str, skill: int, skill_usage: int = SkillUsageType.NotAutoUse,
                        add_to_battle: bool = True, requirements: Requirements = None,
                        check_operator_name: bool = True) -> Operator:
        """
        创建并添加一个干员
        :param name: 干员名称
        :param skill: 要使用的技能
        :param skill_usage: 技能使用类型 参见 SkillUsageType
        :param add_to_battle: 是否要加入自动编队，构造召唤物或关卡道具时等需设为 False
        :param requirements: 可选
        :param check_operator_name: 是否要检查输入的名字是否正确
        :return: 新生成的干员
        """
        new_operator = Operator(name, skill, skill_usage, self, requirements)
        if check_operator_name:
            if not new_operator.check_operator_name():
                raise Exception("干员名称有误，请更改为正确的名称或关闭名称检查")
        if add_to_battle:
            self._add_operator(new_operator)
        return new_operator

    def create_group(self, name: str, *operators: Operator) -> Group:
        return self._create_group(name, True, *operators)

    def create_group_without_name_check(self, name: str, *operators: Operator) -> Group:
        return self._create_group(name, False, *operators)

    def _create_group(self, name: str, check_operator_name: bool = True, *operators: Operator) -> Group:
        """
        创建并添加一个群组
        :param name: 群组名
        :param operators: 所包含的干员，构造的时候 battle 一项置 None 即可
        :param check_operator_name: 是否要检查输入的名字是否正确
        :return: 新创建的群组
        """
        if check_operator_name:
            for operator in operators:
                if not operator.check_operator_name():
                    raise Exception("干员名称有误，请更改为正确的名称或关闭名称检查")
        new_group = Group(name, self, operators)
        self._add_group(new_group)
        return new_group

    def _add_operator(self, operator: Operator):
        self._operators.append(operator)

    def _add_group(self, group: Group):
        self._groups.append(group)

    def add_action(self, action: Action):
        if self._enable_haisi_doc:
            action.add_haisi_doc()
        self._actions.append(action)

    def speed_up(self):
        """
        调整为二倍速
        :return:
        """
        if not self._speedup:
            self._speed_change()
            self._speedup = True

    def speed_down(self):
        """
        调整为一倍速
        :return:
        """
        if self._speedup:
            self._speed_change()
            self._speedup = False

    def _speed_change(self):
        self.add_action(Action(ActionType.SpeedUp))

    def save_as_json(self, file_name: str = None, stage_name: str = None):
        """
        保存为 json 文件，生成在同目录下
        :param file_name: 要保存的文件名
        :param stage_name: 手动指定关卡名
        :return:
        """
        if file_name is None:
            file_name = self._last_file_name = self._generate_file_name(self._stage_name) \
                if stage_name is None else self._generate_file_name(stage_name)
        else:
            self._last_file_name = file_name
        with open(file_name, 'w', encoding='utf-8') as f:
            json.dump(self.to_dict(), f, ensure_ascii=False)

    def _generate_file_name(self, stage_name: str) -> str:
        res = stage_name
        res += "-"
        res += "+".join([operator.name for operator in self._operators])
        res += ".json"
        return res.replace("/", "_")  # 防止文件名中含有 / 而使得文件打开失败

    def to_dict(self) -> dict:
        res = {
            "stage_name": self._stage_name,
            "minimum_required": self._version,
            "opers": [operator.to_dict() for operator in self._operators],
            "groups": [group.to_dict() for group in self._groups],
            "actions": [action.to_dict() for action in self._actions],
        }
        if self._doc is not None:
            res["doc"] = self._doc.to_dict()

        return res

    def test(self, enable_debug_output: bool = True):
        """
        自动调用 MeoAssistantArknights 并测试生成自动战斗的文件
        测试的是上次生成的 json 需先调用 save_as_json()
        请自行配置 MeoAssistantArknights 所在地址等
        同时确保能导入 asst.py

        :param enable_debug_output: 是否开启 debug 输出
        :return:
        """
        # MeoAssistantArknights所在的路径
        mma_path = "D:\\Program Files\\MeoAssistantArknights"
        # adb的路径
        adb_path = "D:\\Program Files\\MeoAssistantArknights\\platform-tools\\adb.exe"
        # adb连接的地址
        adb_address = "127.0.0.1:8873"
        try:
            from asst import Asst, Message
        except ImportError:
            import sys
            import pathlib
            sys.path.append(
                str(pathlib.Path.cwd().parent.parent / "src" / "Python"))
            try:
                from asst import Asst, Message
            except ImportError:
                print("asst导入失败，请自行解决，或者下载"
                      " https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/asst.py "
                      "到同目录下")
                return

        @Asst.CallBackType
        def callback(msg, details, arg):
            print(Message(msg), json.loads(details.decode('utf-8')), arg)

        if not Asst.load(mma_path):
            print("加载MeoAssistantArknights失败，请确定路径是否正确")

        asst = Asst(callback) if enable_debug_output else Asst()
        if asst.connect(adb_path, adb_address):
            print('连接成功')
        else:
            print('连接失败')
            print('请检查adb地址或者端口是否有误')
            exit()
        asst.append_task('Copilot', {
            'stage_name': self._stage_name,
            'filename': self._last_file_name,
            'formation': False,
        })
        asst.start()
        while True:
            if "stop" == str(input('>')):
                exit()
