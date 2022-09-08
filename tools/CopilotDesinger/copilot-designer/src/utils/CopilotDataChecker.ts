import CopilotData, {
  Action,
  Group,
  Operator,
  Requirement,
} from "@/interfaces/CopilotData";

/**
 * 操作类型可选值
 */
const actionTypes = [
  "Deploy",
  "Skill",
  "Retreat",
  "SpeedUp",
  "BulletTime",
  "SkillUsage",
  "Output",
  "SkillDaemon",
];

/**
 * 操作类型对照表
 */
const actionTypeMapping = {
  部署: "Deploy",
  技能: "Skill",
  撤退: "Retreat",
  二倍速: "SpeedUp",
  子弹时间: "BulletTime",
  技能用法: "SkillUsage",
  打印: "Output",
  摆完挂机: "SkillDaemon",
};

/**
 * 方向类型可选值
 */
const actionDirections = ["Left", "Right", "Up", "Down", "None"];

/**
 * 方向类型对照表
 */
const actionDirectionMapping = {
  左: "Left",
  右: "Right",
  上: "Up",
  下: "Down",
  无: "None",
};

/**
 * 创建空的战斗数据
 * @returns 空的战斗数据
 */
export function createEmptyCopilotData(): CopilotData {
  return {
    stage_name: "",
    opers: [],
    groups: [],
    actions: [],
    minimum_required: "v4.0",
  };
}

/**
 * 检查战斗数据必填项是否已存在，如果不存在则填入默认值
 * @param data 要检查的战斗数据
 * @returns 检查后的战斗数据
 */
export function checkCopilotData(data: any): CopilotData {
  const emptyData = createEmptyCopilotData();
  data = { ...emptyData, ...data };

  data.stage_name = data.stage_name || emptyData.stage_name;
  data.actions = data.actions || emptyData.actions;
  data.minimum_required = data.minimum_required || emptyData.minimum_required;

  data.opers = checkOperators(data.opers);
  data.groups = checkGroups(data.groups);
  data.actions = checkActions(data.actions);

  return data as CopilotData;
}

/**
 * 检查数值是否在范围内
 * @param value 要检查的数值或空
 * @param min 最小值，为空则不检查
 * @param max 最大值，为空则不检查
 * @returns 如果值在范围内或为空则返回原值，否则返回最小值或最大值
 */
function checkRange(
  value: number | undefined | null,
  min?: number,
  max?: number
): number | undefined | null {
  if (value === undefined || value === null) {
    return value;
  }

  if (min !== undefined && value < min) return min;
  if (max !== undefined && value > max) return max;
  return value;
}

/**
 * 检查字符串是否在范围内
 * @param value 要检查的字符串或空
 * @param options 可选范围
 * @returns 如果字符串在范围内则返回原值，否则返回空
 */
function checkStringOptions(
  value: string | undefined | null,
  options: string[]
): string | undefined | null {
  if (value === undefined || value === null) {
    return value;
  }

  if (options.includes(value)) {
    return value;
  }

  return undefined;
}

/**
 * 检查字符串对照
 * @param value 要检查的字符串或空
 * @param mapping 对照表
 * @returns 如果字符串不为空则返回对照后的字符串，若找不到对照则返回原值，空字符串返回空
 */
function checkStringMapping(
  value: string | undefined | null,
  mapping: { [key: string]: string }
): string | undefined | null {
  if (value === undefined || value === null) {
    return value;
  }

  if (Object.keys(mapping).includes(value)) {
    return mapping[value];
  }

  return value;
}

/**
 * 检查是否为数组
 * @param value 要检查的数组
 * @returns 如果是数组则返回原值，否则返回空
 */
function checkArray(
  value: any | undefined | null
): Array<any> | undefined | null {
  if (value === undefined || value === null) {
    return value;
  }

  if (!Array.isArray(value)) {
    return [];
  }

  return value;
}

/**
 * 检查干员
 * @param opers 要检查的干员列表
 * @returns 检查后的干员列表
 */
function checkOperators(opers: any): Operator[] | undefined | null {
  return checkArray(opers)?.map((oper) => {
    oper.name = oper.name || "";
    oper.skill = checkRange(oper.skill, 1, 3);
    oper.skill_usage = checkRange(oper.skill_usage, 0, 3);
    oper.requirements = checkRequirement(oper.requirements);

    return oper as Operator;
  });
}

/**
 * 检查练度要求
 * @param requirement 要检查的练度要求
 * @returns 检查后的练度要求
 */
function checkRequirement(requirement: any): Requirement | undefined | null {
  if (requirement === undefined || requirement === null) {
    return requirement;
  }

  requirement.elite = checkRange(requirement.elite, 0, 2);
  requirement.level = checkRange(requirement.level, 0, 90);
  requirement.skill_level = checkRange(requirement.skill_level, 0, 10);
  requirement.module = checkRange(requirement.module, 0, 1);
  requirement.potentiality = checkRange(requirement.potentiality, 0, 5);

  return requirement;
}

/**
 * 检查群组列表
 * @param groups 要检查的群组列表
 * @returns 检查后的群组列表
 */
function checkGroups(groups: any): Group[] | undefined | null {
  return checkArray(groups)?.map((group) => {
    group.name = group.name || "";
    group.opers = checkOperators(group.opers);

    return group as Group;
  });
}

/**
 * 检查战斗操作列表
 * @param actions 要检查的战斗操作列表
 * @returns 检查后的战斗操作列表
 */
function checkActions(actions: any): Action[] | undefined | null {
  return checkArray(actions)?.map((action) => {
    action.type = checkStringOptions(
      checkStringMapping(action.type, actionTypeMapping),
      actionTypes
    );
    action.kills = checkRange(action.kills, 0);
    action.cost_changes = checkRange(action.cost_changes, 0);

    if (action.location !== undefined && action.location !== null) {
      if (!Array.isArray(action.location)) {
        action.location = [0, 0];
      }

      if (action.location.length !== 2) {
        action.location = [0, 0];
      }

      if (typeof action.location[0] !== "number") {
        action.location[0] = 0;
      }

      if (typeof action.location[1] !== "number") {
        action.location[1] = 0;
      }

      action.location[0] = checkRange(action.location[0], 0);
      action.location[1] = checkRange(action.location[1], 0);
    }

    action.direction = checkStringOptions(
      checkStringMapping(action.direction, actionDirectionMapping),
      actionDirections
    );

    action.skill_usage = checkRange(action.skill_usage, 0, 3);
    action.pre_delay = checkRange(action.pre_delay, 0);
    action.rear_delay = checkRange(action.rear_delay, 0);

    return action as Action;
  });
}
