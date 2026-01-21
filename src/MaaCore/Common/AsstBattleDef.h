#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "AsstTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"

namespace asst::battle
{
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

    bool operator==(const OperatorRequirements& req) const
    {
        return elite == req.elite && level == req.level && skill_level == req.skill_level && module == req.module;
    }
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

struct OperUsage // 干员用法
{
    std::string name;
    int skill = 0;                                // 技能序号，取值范围 [0, 3]，0时使用默认技能 或 上次编队时使用的技能
    SkillUsage skill_usage = SkillUsage::NotUse;
    int skill_times = 1;                          // 使用技能的次数，默认为 1，兼容曾经的作业
    battle::OperatorRequirements requirements {}; // 练度需求
    OperStatus status = OperStatus::Unchecked;    // 编队状态, 可能有其他更好的位置存储

    bool operator==(const OperUsage& other) const
    {
        return name == other.name && skill == other.skill && skill_usage == other.skill_usage &&
               skill_times == other.skill_times && requirements == other.requirements;
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

enum class Role
{
    Unknown,
    Caster,  // 术士
    Medic,   // 医疗
    Pioneer, // 先锋
    Sniper,  // 狙击
    Special, // 特种
    Support, // 辅助
    Tank,    // 重装
    Warrior, // 近卫
    Drone    // 无人机
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
    std::array<std::string, 3> ranges;
    int rarity = 0;
    LocationType location_type = LocationType::None;
    std::vector<std::string> tokens; // 召唤物名字
};

using AttackRange = std::vector<Point>;
using RoleCounts = std::unordered_map<Role, int>;

namespace copilot
{
using OperUsageGroup = std::pair<std::string, std::vector<OperUsage>>;
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
    std::string name; // 目标名，若 type >= SwitchSpeed, name 为空
    Point location;
    DeployDirection direction = DeployDirection::Right;
    SkillUsage modify_usage = SkillUsage::NotUse;
    int modify_times = 1; // 更改使用技能的次数，默认为 1，兼容曾经的作业
    int pre_delay = 0;
    int post_delay = 0;
    int time_out = INT_MAX; // TODO
    std::string doc;
    std::string doc_color;
    RoleCounts role_counts;
    std::pair<double, double> distance;
    bool skip_if_not_ready = false; // 跳过使用未准备好的技能，主要用于关闭技能的场景 (试验性功能)
    int elapsed_time = 0;           // 全局计时条件 (试验性功能)
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
