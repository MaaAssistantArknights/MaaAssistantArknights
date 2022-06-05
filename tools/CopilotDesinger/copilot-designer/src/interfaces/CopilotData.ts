// 自动战斗数据
export default interface CopilotData {
  // 关卡名，必选。除危机合约外，均为关卡中文名
  stage_name: string;
  // 指定干员
  opers: Operator[];
  // 群组
  groups: Group[];
  // 战斗中的操作。有序，执行完前一个才会去执行下一个。必选
  actions: Action[];
  // 最低要求 maa 版本号，必选。保留字段，暂未实现
  minimum_required: string;
  // 描述，可选
  doc?: Documentation;
}

// 干员
export interface Operator {
  // 干员名
  name: string;
  // 技能序号。可选，默认 1，取值范围 [1, 3]
  skill?: 0 | 1 | 2 | 3;
  // 技能用法。可选，默认 0
  // 0 - 不自动使用（依赖 "actions" 字段）
  // 1 - 好了就用，有多少次用多少次（例如干员 棘刺 3 技能、桃金娘 1 技能等）
  // 2 - 好了就用，仅使用一次（例如干员 山 2 技能）
  // 3 - 自动判断使用时机（画饼.jpg）
  // 如果是全自动的技能，填 0
  skill_usage?: number;
  // 练度要求。可选，默认为空
  requirements?: Requirement[];
}

// 练度
export interface Requirement {
  // 精英化等级。可选，默认为 0, 不要求精英化等级
  elite?: 0 | 1 | 2;
  // 干员等级。可选，默认为 0
  level?: number;
  // 技能等级。可选，默认为 0
  skill_level?: number;
  // 模组编号。可选，默认为 0
  module?: number;
  // 潜能要求。可选，默认为 0
  potentiality?: 0 | 1 | 2 | 3;
}

// 群组
export interface Group {
  // 群组名，必选。自己随便取名字，与下面 deploy 中的 name 对应起来就行
  name: string;
  // 干员任选其一，无序，会优先选练度高的
  opers: Operator[];
}

// 操作
export interface Action {
  // 操作类型，可选，默认 "Deploy"
  type?:
    | "Deploy"
    | "Skill"
    | "Retreat"
    | "SpeedUp"
    | "BulletTime"
    | "SkillUsage"
    | "部署"
    | "技能"
    | "撤退"
    | "二倍速"
    | "子弹时间"
    | "技能用法";
  // 击杀数条件，如果没达到就一直等待。可选，默认为 0，直接执行
  kills?: number;
  // 费用变化量，如果没达到就一直等待。可选，默认为 0，直接执行
  cost_changes?: number;
  // 干员名 或 群组名， type 为 "部署" | "技能" | "撤退" 之一时必选
  name?: string;
  // 部署干员的位置。
  // type 为 "部署" 时必选。
  // type 为 `撤退` 时可选，
  // 仅推荐有多个同名召唤物时，不填写 name, 并使用 location 进行撤退。
  // 正常部署的干员请使用 name 进行撤退
  location?: [number, number];
  // 部署干员的干员朝向。 type 为 "部署" 时必选
  direction?:
    | "Left"
    | "Right"
    | "Up"
    | "Down"
    | "None"
    | "左"
    | "右"
    | "上"
    | "下"
    | "无";
  // 修改技能用法。当 type 为 "技能用法" 时必选
  skill_usage?: number;
  // 前置延时。可选，默认 0, 单位毫秒
  pre_delay?: number;
  // 后置延时。可选，默认 0, 单位毫秒
  rear_delay?: number;
  // 描述，可选。会显示在界面上，没有实际作用
  doc?: string;
  // 描述文字的颜色，可选，默认灰色。会显示在界面上，没有实际作用。
  doc_color?: string;
}

// 描述
export interface Documentation {
  title: string;
  title_color?: string;
  details?: string;
  details_color?: string;
}

export function createEmptyCopilotData(): CopilotData {
  return {
    stage_name: "",
    opers: [],
    groups: [],
    actions: [],
    minimum_required: "",
  };
}
