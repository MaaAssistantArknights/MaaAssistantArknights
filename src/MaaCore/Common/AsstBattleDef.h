#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "AsstTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"

namespace asst::battle
{
enum class Role
{
    Unknown,
    Pioneer, // 先锋
    Warrior, // 近卫
    Tank,    // 重装
    Sniper,  // 狙击
    Caster,  // 术士
    Medic,   // 医疗
    Support, // 辅助
    Special, // 特种
    Drone    // 无人机
};

enum class SubRole
{
    Unknown,

    Pioneer_Pioneer,          // 尖兵; 挡2, 德克萨斯
    Pioneer_Charger,          // 冲锋手; 击杀回费, 风笛, 苇草
    Pioneer_Tactician,        // 战术家; 带一个战术点(召唤物), 缪尔赛思
    Pioneer_Bearer,           // 执旗手; 投锋, 桃金娘
    Pioneer_Agent,            // 情报官; 可远程攻击, 伊内丝
    Pioneer_Counsellor,       // 策士; 支援待部署, 凛御银灰

    Warrior_Centurion,        // 强攻手; 可同时攻击阻挡的所有敌人, 煌, 幽灵鲨
    Warrior_Fighter,          // 斗士; 挡1, 山, 重岳
    Warrior_ArtsFighter,      // 术战者; 攻击造成法术伤害, 史尔特尔, 星极; 鹰语ArtsFghter
    Warrior_Instructor,       // 教官; 攻击到较远敌人, 帕拉斯, 杜宾
    Warrior_Lord,             // 领主; 远程攻击伤害不衰减, 银灰, 棘刺
    Warrior_Sword,            // 剑豪; 普攻造成两次伤害, 陈, 艾丽妮
    Warrior_Musha,            // 武者; 无法被治疗, 攻击回血, 赫拉格, 左乐
    Warrior_Fearless,         // 无畏者; 挡1, 斯卡蒂, 耀骑士临光
    Warrior_Reaper,           // 收割者; 无法被治疗, AOE攻击回血, 圣约送葬人, 羽毛笔
    Warrior_Liberator,        // 解放者; 不攻击时积攒攻击力, 玛恩纳, 龙舌兰; 鹰语Librator
    Warrior_Crusher,          // 重剑手; 攻击无视目标一定防御, 赫德雷, 乌尔比安
    Warrior_Hammer,           // 撼地者; 目标周围受到一半物理AOE, 佩佩, 怒潮凛冬
    Warrior_Primguard,        // 本源近卫; 挡2元素伤害, 聆音
    Warrior_Mercenary,        // 佣兵; 消耗费用强化自身, 雷狼龙S空爆, 哈蒂娅

    Tank_Protector,           // 铁卫; 挡3, 星熊, 年
    Tank_Guardian,            // 守护者; 奶盾, 塞雷娅, 黍
    Tank_Unyield,             // 不屈者; 无法被治疗, 泥岩, 斥罪
    Tank_ArtsProtector,       // 驭法铁卫; 开技能时普攻法伤, 斩业星熊, 石棉
    Tank_Duelist,             // 决战者; 阻挡回技力, 森蚺, 极光
    Tank_Fortress,            // 要塞; 不阻挡时远程攻击, 号角, 灰毫
    Tank_ShotProtector,       // 哨戒铁卫; 挡3远程攻击, 涤火杰西卡, 雷蛇
    Tank_PrimProtector,       // 本源铁卫; 挡3元素伤害, 余, 珊比

    Sniper_FastShot,          // 速射手; 优先攻击空中单位, 能天使, 空弦
    Sniper_CloseRange,        // 重射手; 高精度的近距离射击, 黑, 鸿雪
    Sniper_AoeSniper,         // 炮手; 攻击造成群体物理伤害, W, 菲亚梅塔
    Sniper_LongRange,         // 神射手; 优先攻击防御力最低的敌人, 远牙, 守林人
    Sniper_ReaperRange,       // 散射手; 攻击前方一横排敌人伤害提升, 假日威龙陈, 送葬人
    Sniper_SiegeSniper,       // 攻城手; 优先攻击重量最重的敌人, 早露, 提丰
    Sniper_Bombarder,         // 投掷手; 对小范围地面敌人造成两次物理伤害, 维什戴尔, 迷迭香
    Sniper_Hunter,            // 猎手; 攻击消耗子弹, 攻击力提升至120%, 莱伊, 冰酿
    Sniper_LoopShooter,       // 回环射手; 持有回旋投射物时才能攻击, 娜仁图亚, 水灯心
    Sniper_SkyBreaker,        // 裂空炮手; 起飞后只攻击空中敌人, 天空盒

    Caster_CoreCaster,        // 中坚术师; 攻击造成法术伤害, 艾雅法拉, 刻俄柏
    Caster_SplashCaster,      // 扩散术师; 攻击造成群体法术伤害, 夕, 莫斯提马
    Caster_Funnel,            // 驭械术师; 操作浮游单元造成法伤, 荒芜拉普兰德, 澄闪
    Caster_Phalanx,           // 阵法术师; 平时不攻击, 技能开启时群体法伤, 林, 卡涅利安
    Caster_Mystic,            // 秘术师; 无目标时储存能量一齐发射, 黑键, 维伊
    Caster_Chain,             // 链术师; 在3个敌人间跳跃并短暂停顿, 异客, 星源
    Caster_BlastCaster,       // 轰击术师; 超远距离群体法伤, 伊芙利特, 谬因
    Caster_PrimCaster,        // 本源术师; 可造成元素伤害, 烛煌, 妮芙
    Caster_SoulCaster,        // 塑灵术师; 击倒敌人生成召唤物, 死芒, 特克诺

    Medic_Physician,          // 医师; 恢复友方单位生命, 凯尔希, 闪灵
    Medic_RingHealer,         // 群愈师; 同时恢复三个友方单位的生命, 夜莺, 白面鸮
    Medic_Healer,             // 疗养师; 治疗范围大, 远距离治疗量降至80%, 流明, 锡兰
    Medic_WanderMedic,        // 行医; 恢复生命与元素损伤, 纯烬艾雅法拉, 蜜莓
    Medic_IncantationMedic,   // 咒愈师; 攻击造成法伤并为友方治疗50%伤害的生命, 焰影苇草, 濯尘芙蓉
    Medic_ChainHealer,        // 链愈师; 在3个友方单位间跳跃治疗, 明椒, 莎草
    Medic_Watchman,           // 守望者; 恢复生命并可起飞, 凯尔希·思衡托, 风絮

    Support_Slower,           // 凝滞师; 攻击造成法伤并短暂停顿, 铃兰, 安洁莉娜
    Support_Underminer,       // 削弱者; 攻击造成法术伤害, 灵知, 初雪
    Support_Bard,             // 吟游者; 不攻击, 持续恢复范围内友军生命, 浊心斯卡蒂, 空
    Support_Blessing,         // 护佑者; 攻击法伤, 技能开启后转为治疗, 淬羽赫默, 月禾
    Support_Summoner,         // 召唤师; 使用召唤物协助作战, 令, 麦哲伦
    Support_Craftsman,        // 工匠; 阻挡2, 使用支援装置协助作战, 白铁, 掠风
    Support_Ritualist,        // 巫役; 攻击造成法伤和元素损伤, 酒神, 塑心
    Support_SupportiveRanger, // 游击手; 使用触发型效果协助作战, 佩德洛

    Special_Executor,         // 处决者; 再部署时间大幅度减少, 缄默德克萨斯, 傀影
    Special_Pusher,           // 推击手; 同时攻击阻挡的所有敌人, 可放远程位, 温蒂, 食铁兽
    Special_Stalker,          // 伏击客; 范围群伤, 50%物法闪避, 水月, 狮蝎
    Special_HookMaster,       // 钩索师; 技能使敌人位移, 可放远程位, 歌蕾蒂娅, 崖心
    Special_Geek,             // 怪杰; 自身生命不断流失, 阿, 新约能天使
    Special_Merchant,         // 行商; 在场时每3秒消耗3点部署费用, 老鲤, 孑
    Special_Traper,           // 陷阱师; 使用陷阱协助作战, 多萝西, 罗宾
    Special_DollKeeper,       // 傀儡师; 致命伤时切换替身作战, 归溟幽灵鲨, 风丸
    Special_Alchemist,        // 炼金师; 投掷炼金单元协助作战, 引星棘刺, 锡人
    Special_SkyWalker,        // 巡空者; 起飞后阻挡2个飞行敌人, 予愿安洁莉娜, 云迹
};

// 统一变量名：
// loc, location, 表示格子坐标，例如 [1, 1], [5, 5]
// pos, position, 表示像素坐标，例如 [1280, 720], [500, 300]

enum class SkillUsage // 技能用法
{
    NotUse = 0,       // 不自动使用
    Possibly = 1,     // 有就用，例如干员 棘刺 3 技能
    Times = 2,        // 用 X 次，例如干员 山 2 技能用 1 次、重岳 3 技能用 5 次，由 "skill_times" 字段控制
    InTime = 3,       // 自动判断使用时机，画饼.jpg
    TimesUsed         // 已经使用了 X 次
};

// 干员练度需求
struct OperatorRequirements
{
    int elite = -1;       // 精英化等级
    int level = -1;       // 干员等级
    int skill_level = -1; // 技能等级
    int module = -1;      // 模组编号 -1: 不切换模组 / 无要求, 0: 不使用模组, 1: 模组χ, 2: 模组γ, 3: 模组α, 4: 模组Δ
                          // int potentiality = -1; // 潜能要求

    auto operator<=>(const OperatorRequirements&) const = default;
};

// 干员编队状态
enum class OperStatus
{
    Unchecked,   // 未检查, 默认值; 理论上仅group中有干员选中后, 其余干员才会保留该状态
    Selected,    // 已编入
    Missing,     // 缺失
    Unavailable, // 不可用, 要求不达标
    // Unknown,     // 未知状态
};

struct OperUsage                                  // 干员用法
{
    battle::Role role = battle::Role::Unknown;    // 干员职业
    std::string name;
    int skill = 0;                                // 技能序号，取值范围 [0, 3]，0时使用默认技能 或 上次编队时使用的技能
    SkillUsage skill_usage = SkillUsage::NotUse;
    int skill_times = 1;                          // 使用技能的次数，默认为 1，兼容曾经的作业
    battle::OperatorRequirements requirements {}; // 练度需求
    OperStatus status = OperStatus::Unchecked;    // 编队状态, 可能有其他更好的位置存储

    auto operator<=>(const OperUsage& other) const
    {
        return std::tie(role, name, skill, skill_usage, skill_times, requirements) <=>
               std::tie(other.role, other.name, other.skill, other.skill_usage, other.skill_times, other.requirements);
    }

    bool operator==(const OperUsage& other) const
    {
        return std::tie(role, name, skill, skill_usage, skill_times, requirements) ==
               std::tie(other.role, other.name, other.skill, other.skill_usage, other.skill_times, other.requirements);
    }
};

enum class DeployDirection
{
    Right = 0,
    Down = 1,
    Left = 2,
    Up = 3,
    None = 4 // 没有方向，通常是无人机之类的
};

inline static Role get_role_type(const std::string& role_name)
{
    // clang-format off
    static const std::unordered_map<std::string, Role> NameToRole = {
        { "WARRIOR", Role::Warrior },    { "Warrior", Role::Warrior },    { "warrior", Role::Warrior },
        { "GUARD", Role::Warrior },      { "Guard", Role::Warrior },      { "guard", Role::Warrior },
        { "近卫", Role::Warrior },

        { "PIONEER", Role::Pioneer },    { "Pioneer", Role::Pioneer },    { "pioneer", Role::Pioneer },
        { "VANGUARD", Role::Pioneer },   { "Vanguard", Role::Pioneer },   { "vanguard", Role::Pioneer },
        { "先锋", Role::Pioneer },

        { "MEDIC", Role::Medic },        { "Medic", Role::Medic },        { "medic", Role::Medic },
        { "医疗", Role::Medic },

        { "TANK", Role::Tank },          { "Tank", Role::Tank },          { "tank", Role::Tank },
        { "DEFENDER", Role::Tank },      { "Defender", Role::Tank },      { "defender", Role::Tank },
        { "重装", Role::Tank },          { "坦克", Role::Tank },

        { "SNIPER", Role::Sniper },      { "Sniper", Role::Sniper },      { "sniper", Role::Sniper },
        { "狙击", Role::Sniper },

        { "CASTER", Role::Caster },      { "Caster", Role::Caster },      { "caster", Role::Caster },
        { "术师", Role::Caster },        { "术士", Role::Caster },        { "法师", Role::Caster },

        { "SUPPORT", Role::Support },    { "Support", Role::Support },    { "support", Role::Support },
        { "SUPPORTER", Role::Support },  { "Supporter", Role::Support },  { "supporter", Role::Support },
        { "辅助", Role::Support },       { "支援", Role::Support },

        { "SPECIAL", Role::Special },    { "Special", Role::Special },    { "special", Role::Special },
        { "SPECIALIST", Role::Special }, { "Specialist", Role::Special }, { "specialist", Role::Special },
        { "特种", Role::Special },

        { "DRONE", Role::Drone },        { "Drone", Role::Drone },        { "drone", Role::Drone },
        { "SUMMON", Role::Drone },       { "Summon", Role::Drone },       { "summon", Role::Drone },
        { "无人机", Role::Drone },       { "召唤物", Role::Drone },
    }; // clang-format on
    if (auto iter = NameToRole.find(role_name); iter != NameToRole.end()) {
        return iter->second;
    }
    return Role::Unknown;
}

// 临时兼容性解析; 后续将拆分token及trap
inline static Role parse_role_type(const std::string& role_name)
{
    static const std::unordered_map<std::string, battle::Role> RoleMap = {
        { "CASTER", battle::Role::Caster }, { "MEDIC", battle::Role::Medic },     { "PIONEER", battle::Role::Pioneer },
        { "SNIPER", battle::Role::Sniper }, { "SPECIAL", battle::Role::Special }, { "SUPPORT", battle::Role::Support },
        { "TANK", battle::Role::Tank },     { "WARRIOR", battle::Role::Warrior },
    };
    if (auto iter = RoleMap.find(role_name); iter != RoleMap.end()) {
        return iter->second;
    }
    return battle::Role::Drone;
}

inline static Role parse_role_type_copilot(const std::string& role_name)
{
    static const std::unordered_map<std::string, battle::Role> RoleMap = {
        { "caster", battle::Role::Caster }, { "medic", battle::Role::Medic },     { "pioneer", battle::Role::Pioneer },
        { "sniper", battle::Role::Sniper }, { "special", battle::Role::Special }, { "support", battle::Role::Support },
        { "tank", battle::Role::Tank },     { "warrior", battle::Role::Warrior },
    };
    if (auto iter = RoleMap.find(role_name); iter != RoleMap.end()) {
        return iter->second;
    }
    return battle::Role::Unknown;
}

enum class OperPosition
{
    None,
    Blocking,   // 阻挡单位
    AirDefense, // 对空单位
};

enum class LocationType
{
    Invalid = -1,
    None = 0,
    Melee = 1,
    Ranged = 2,
    All = 3
};

inline static LocationType get_role_usual_location(const Role& role)
{
    switch (role) {
    case Role::Warrior:
    case Role::Pioneer:
    case Role::Tank:
    case Role::Special:
    case Role::Drone:
        return LocationType::Melee;
    case Role::Medic:
    case Role::Sniper:
    case Role::Caster:
    case Role::Support:
        return LocationType::Ranged;
    default:
        return LocationType::None;
    }
}

// ————————————————————————————————————————————————————————————————
// 招募相关
// ————————————————————————————————————————————————————————————————
/// <summary>
/// 编队/招募时对所需干员模组的要求。
/// </summary>
enum class OperModule
{
    /// <summary>
    /// 无指定模组
    /// </summary>
    Unspecified = -1,

    /// <summary>
    /// 基础模组/无模组。
    /// </summary>
    Original = 0,

    /// <summary>
    /// Chi 模组。
    /// </summary>
    Chi,

    /// <summary>
    /// Upsilon 模组。
    /// </summary>
    Upsilon,

    /// <summary>
    /// Delta 模组。
    /// </summary>
    Delta,

    /// <summary>
    /// Alpha 模组。
    /// </summary>
    Alpha,

    /// <summary>
    /// Beta 模组。
    /// </summary>
    Beta,
};

/// <summary>
/// 编队/招募需要的干员。
/// </summary>
struct RequiredOper
{
    /// <summary>
    /// 所需干员职业。
    /// </summary>
    Role role = Role::Unknown;

    /// <summary>
    /// 所需干员名称。
    /// </summary>
    std::string name;

    /// <summary>
    /// 所需干员最低精英阶段，当且仅当 <c>level != 0</c> 时有效。
    /// 为 0–2 的整数，分别表示精英阶段 1–2。
    /// </summary>
    int elite = 0;

    /// <summary>
    /// 所需干员最低等级。
    /// 精英阶段高于 <c>elite</c> 的干员不受此要求限制。
    /// 为 0–90 的整数，其中 0 表示无要求，1–90 分别表示 1–90 级。
    /// </summary>
    int level = 0;

    /// <summary>
    /// 所需干员携带技能。为 0–3 的整数，其中 0 表示无需指定技能，1–3 分别表示一、二、三技能。
    /// </summary>
    int skill = 0;

    /// <summary>
    /// 所需干员最低技能等级。
    /// 仅在 <c>RequiredOper::skill != 0</c> 时有效。
    /// 为 0–10 的整数，其中 0 表示无要求，1–7 分别表示 1–7 级，8–10 分别表示专精等级 1–3 级。
    /// </summary>
    int skill_level = 0;

    /// <summary>
    /// 所需干员携带模组。
    /// </summary>
    OperModule module = OperModule::Unspecified;

    /// <summary>
    /// 所需干员携带模组的最低等级。
    /// 仅在 <c>module</c> 不为 <c>OperModule::Unspecified</c> 或 <c>OperModule::Original</c> 时有效。
    /// 为 0–3 的整数，其中 0 表示无要求，1–3 分别表示 1–3 级。
    /// </summary>
    int module_level = 0;

    /// <summary>
    /// 所需干员最低潜能。
    /// 为 0–6 的整数，其中 0 表示无要求，1–6 分别表示 1–6 潜。
    /// </summary>
    int potential = 0;
};

/// <summary>
/// 好友关系。
/// </summary>
enum class Friendship
{
    /// <summary>
    /// 陌生人。
    /// </summary>
    Stranger = 0,

    /// <summary>
    /// 好友。
    /// </summary>
    Friend,

    /// <summary>
    /// 挚友。
    /// </summary>
    BestFriend,
};

/// <summary>
/// 备选助战干员。
/// </summary>
struct SupportUnit
{
    cv::Mat templ;

    /// <summary>
    /// 助战干员名称。
    /// </summary>
    std::string name;

    /// <summary>
    /// 助战干员精英化阶段。
    /// </summary>
    int elite = 0;

    /// <summary>
    /// 助战干员等级。
    /// </summary>
    int level = 0;

    /// <summary>
    /// 助战干员潜能。
    /// </summary>
    int potential = 0;

    /// <summary>
    /// 是否可以选择模组。
    /// </summary>
    bool module_enabled = false;

    /// <summary>
    /// 与助战干员提供者之间的亲密度。
    /// </summary>
    Friendship friendship = Friendship::Stranger;
    // ———————— 以下字段仅在集成战略中有效 ————————
    // int hope = 0;                  // 希望消耗
    // int elite_after_promotion = 0; // 进阶后精英化阶段，仅在集成战略中有效，
    // int level_after_promotion = 0; // 进阶后等级，仅在集成战略中有效，
};

/// <summary>
/// 根据 <c>role</c> 对干员名 <c>literal_name</c> 进行消歧义，目前仅用于区分不同升变形态下的阿米娅。
/// </summary>
inline static std::string canonical_oper_name(battle::Role role, const std::string& literal_name)
{
    using battle::Role;
    static const std::unordered_map<std::pair<Role, std::string>, std::string, std::pair_hash<Role, std::string>>
        CanonicalOperNameDict {
            { { Role::Caster, "阿米娅" }, "阿米娅" },
            { { Role::Warrior, "阿米娅" }, "阿米娅-WARRIOR" },
            { { Role::Medic, "阿米娅" }, "阿米娅-MEDIC" },
        };

    if (const auto iter = CanonicalOperNameDict.find({ role, literal_name }); iter != CanonicalOperNameDict.end()) {
        return iter->second;
    }

    return literal_name;
}

// ————————————————————————————————————————————————————————————————

struct DeploymentOper
{
    size_t index = 0;
    Role role = Role::Unknown;
    int cost = 0;
    bool available = false;
    bool cooling = false;
    Rect rect;
    cv::Mat avatar;
    std::string name;
    LocationType location_type = LocationType::None;
    bool is_usual_location = false; // 用于判断地面辅助，高台先锋（unusual 时此值为 false）等
};

struct OperProps
{
    std::string id;
    std::string name;
    std::string name_en;
    std::string name_jp;
    std::string name_kr;
    std::string name_tw;
    Role role = Role::Unknown;
    SubRole sub_role = SubRole::Unknown;
    std::array<std::string, 3> ranges;
    int rarity = 0;                  // 稀有度 1-6
    int sort_index = 0;              // 排序索引
    LocationType location_type = LocationType::None;
    std::vector<std::string> tokens; // 召唤物名字
};

using AttackRange = std::vector<Point>;
using RoleCounts = std::unordered_map<Role, int>;

namespace copilot
{

struct OperUsageGroup
{
    std::string name;  // 干员组名
    int elite_min = 0; // 组内干员的最小精英化等级
    int level_min = 0; // 组内干员的最小等级
    std::vector<asst::battle::OperUsage> opers;
};

using OperUsageGroups = std::vector<OperUsageGroup>;

enum class ActionType
{
    Deploy,         // 部署干员
    UseSkill,       // 开技能
    Retreat,        // 撤退干员
    SkillUsage,     // 技能用法
    SwitchSpeed,    // 切换二倍速
    BulletTime,     // 使用 1/5 的速度
    Output,         // 仅输出，什么都不操作，界面上也不显示
    SkillDaemon,    // 什么都不做，有技能开技能，直到战斗结束
    ResetStopwatch, // 重置全局计时器 (试验性功能)

    /* for TRN */
    MoveCamera, // 引航者试炼，移动镜头

    /* for SSS */
    DrawCard,         // “调配干员”
    CheckIfStartOver, // 检查如果没有某干员则退出重开
};

struct Action
{
    int kills = 0;
    int costs = 0;
    int cost_changes = 0;
    int cooling = 0;
    ActionType type = ActionType::Deploy;
    battle::Role role = battle::Role::Unknown; // 目标职业
    std::string name;                          // 目标名，若 type >= SwitchSpeed, name 为空
    Point location;
    DeployDirection direction = DeployDirection::Right;
    SkillUsage modify_usage = SkillUsage::NotUse;
    int modify_times = 1; // 更改使用技能的次数，默认为 1，兼容曾经的作业
    int pre_delay = 0;
    int post_delay = 0;
    int timeout_ms = -1; // 动作超时时间, 单位ms
    std::string doc;
    std::string doc_color;
    RoleCounts role_counts;
    std::pair<double, double> distance;
    int elapsed_time = 0; // 全局计时条件 (试验性功能)
};

struct BasicInfo
{
    std::string stage_name;
    std::string minimum_required;
    std::string title;
    std::string title_color;
    std::string details;
    std::string details_color;
};

struct CombatData // 作业 JSON 数据
{
    BasicInfo info;
    OperUsageGroups groups;
    std::vector<Action> actions;
};
} // namespace copilot

namespace sss // 保全派驻
{
struct Strategy
{
    std::optional<std::string> core;
    RoleCounts tool_men; // 初始需要多少工具人
    Point location;
    DeployDirection direction = DeployDirection::None;

    bool all_deployed = false; // 当前 strategy 是否已经完成
};

struct CombatData : public copilot::CombatData
{
    std::vector</*const*/ Strategy> strategies; // 按顺序存储的 strategies
    bool draw_as_possible = false;
    int retry_times = 0;
    std::vector<std::string> order_of_drops;
};

enum class EquipmentType
{
    NotChoose,
    A,
    B,
};

struct CompleteData
{
    copilot::BasicInfo info;

    std::string buff;
    std::vector<EquipmentType> equipment;
    std::string strategy;

    copilot::OperUsageGroups groups;
    RoleCounts tool_men;
    std::vector<std::string> order_of_drops;
    std::unordered_set<std::string> blacklist;

    std::unordered_map<std::string, CombatData> stages_data;
};
}

namespace roguelike
{
struct ReplacementHome
{
    Point location;
    DeployDirection direction = DeployDirection::Right;
};

struct DeployInfoWithRank
{
    Point location;
    DeployDirection direction = DeployDirection::None;
    int rank = 0;
    int kill_lower_bound = 0;
    int kill_upper_bound = 9999;
};

struct ForceDeployDirection
{
    DeployDirection direction = DeployDirection::Right;
    std::unordered_set<Role> role = {};
};

struct CombatData
{
    std::string stage_name;
    std::vector<ReplacementHome> replacement_home;
    std::unordered_set<Point> blacklist_location;
    std::unordered_map<Point, ForceDeployDirection> force_deploy_direction;
    std::array<Role, 9> role_order = {};
    bool use_dice_stage = true;
    int stop_deploy_blocking_num = INT_MAX;
    int force_deploy_air_defense_num = 0;
    bool force_ban_medic = false;
    std::unordered_map<std::string, std::vector<DeployInfoWithRank>> deploy_plan;
    std::vector<DeployInfoWithRank> retreat_plan;
};

struct Recruitment
{
    std::string name;
    Rect rect;
    int elite = 0;
    int level = 0;
};

enum class SupportAnalyzeMode
{
    ChooseSupportBtn,
    AnalyzeChars,
    RefreshSupportBtn
};

struct RecruitSupportCharInfo
{
    Recruitment oper_info;
    bool is_friend = false; // 是否为好友助战
    int max_elite = 0;      // 两次招募后的实际精英化与等级
    int max_level = 0;
};

struct RefreshSupportInfo
{
    Rect rect;
    bool in_cooldown = false;
    int remain_secs = 0; // 刷新冷却时间
};
} // namespace roguelike
} // namespace asst::battle

namespace asst
{
inline std::string enum_to_string(asst::battle::Role role, bool en = false)
{
    using asst::battle::Role;
    static const std::unordered_map<Role, std::pair<std::string, std::string>> RoleToName {
        { Role::Warrior, { "近卫", "Warrior" } }, { Role::Pioneer, { "先锋", "Pioneer" } },
        { Role::Medic, { "医疗", "Medic" } },     { Role::Tank, { "重装", "Tank" } },
        { Role::Sniper, { "狙击", "Sniper" } },   { Role::Caster, { "术师", "Caster" } },
        { Role::Support, { "辅助", "Support" } }, { Role::Special, { "特种", "Special" } },
        { Role::Drone, { "无人机", "Drone" } },
    };

    if (auto iter = RoleToName.find(role); iter != RoleToName.end()) {
        return en ? iter->second.second : iter->second.first;
    }

    return "Unknown";
}

inline std::string enum_to_string(const battle::OperModule module)
{
    using OperModule = battle::OperModule;
    static const std::unordered_map<OperModule, std::string> OPER_MODULE_STR_MAP {
        { OperModule::Unspecified, "Unspecified" },
        { OperModule::Original, "Original" },
        { OperModule::Chi, "Chi" },
        { OperModule::Upsilon, "Upsilon" },
        { OperModule::Delta, "Delta" },
        { OperModule::Alpha, "Alpha" },
        { OperModule::Beta, "Beta" },
    };

    if (const auto iter = OPER_MODULE_STR_MAP.find(module); iter != OPER_MODULE_STR_MAP.end()) {
        return iter->second;
    }

    return "Unknown";
}
} // namespace asst

namespace asst::battle
{
struct OperNameTag
{
    Role role = Role::Unknown; // 干员职业
    std::string name;          // 干员名

    auto operator<=>(const OperNameTag&) const = default;

    std::string to_string() const { return "(" + enum_to_string(role) + ", " + name + ")"; }

    explicit operator std::string() const { return to_string(); }
};
}

namespace std
{
template <>
struct hash<asst::battle::OperNameTag>
{
    std::size_t operator()(const asst::battle::OperNameTag& k) const noexcept
    {
        return std::hash<std::string> {}(k.name) ^ (std::hash<int> {}(static_cast<int>(k.role)) << 1);
    }
};
}
