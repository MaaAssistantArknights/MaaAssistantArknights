#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "AsstTypes.h"
#include "Utils/NoWarningCVMat.h"

namespace asst::battle
{
// 统一变量名：
// loc, location, 表示格子坐标，例如 [1, 1], [5, 5]
// pos, position, 表示像素坐标，例如 [1280, 720], [500, 300]

enum class SkillUsage // 技能用法
{
    NotUse = 0,       // 不自动使用
    Possibly = 1,     // 有就用，例如干员 棘刺 3 技能
    Times = 2, // 用 X 次，例如干员 山 2 技能用 1 次、重岳 3 技能用 5 次，由 "skill_times" 字段控制
    InTime = 3, // 自动判断使用时机，画饼.jpg
    TimesUsed   // 已经使用了 X 次
};

struct OperUsage // 干员用法
{
    std::string name;
    int skill = 0; // 技能序号，取值范围 [0, 3]，0时使用默认技能 或 上次编队时使用的技能
    SkillUsage skill_usage = SkillUsage::NotUse;
    int skill_times = 1; // 使用技能的次数，默认为 1，兼容曾经的作业
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
    Caster,
    Medic,
    Pioneer,
    Sniper,
    Special,
    Support,
    Tank,
    Warrior,
    Drone
};

inline static Role get_role_type(const std::string& role_name)
{
    static const std::unordered_map<std::string, Role> NameToRole = {
        { "warrior", Role::Warrior },    { "WARRIOR", Role::Warrior },    { "Warrior", Role::Warrior },
        { "近卫", Role::Warrior },       { "GUARD", Role::Warrior },      { "guard", Role::Warrior },
        { "Guard", Role::Warrior },

        { "pioneer", Role::Pioneer },    { "PIONEER", Role::Pioneer },    { "Pioneer", Role::Pioneer },
        { "先锋", Role::Pioneer },       { "VANGUARD", Role::Pioneer },   { "vanguard", Role::Pioneer },
        { "Vanguard", Role::Pioneer },

        { "medic", Role::Medic },        { "MEDIC", Role::Medic },        { "Medic", Role::Medic },
        { "医疗", Role::Medic },

        { "tank", Role::Tank },          { "TANK", Role::Tank },          { "Tank", Role::Tank },
        { "重装", Role::Tank },          { "DEFENDER", Role::Tank },      { "defender", Role::Tank },
        { "Defender", Role::Tank },      { "坦克", Role::Tank },

        { "sniper", Role::Sniper },      { "SNIPER", Role::Sniper },      { "Sniper", Role::Sniper },
        { "狙击", Role::Sniper },

        { "caster", Role::Caster },      { "CASTER", Role::Caster },

        { "Caster", Role::Caster },      { "术师", Role::Caster },        { "术士", Role::Caster },
        { "法师", Role::Caster },

        { "support", Role::Support },    { "SUPPORT", Role::Support },    { "Support", Role::Support },
        { "supporter", Role::Support },  { "SUPPORTER", Role::Support },  { "Supporter", Role::Support },
        { "辅助", Role::Support },       { "支援", Role::Support },

        { "special", Role::Special },    { "SPECIAL", Role::Special },    { "Special", Role::Special },
        { "特种", Role::Special },       { "SPECIALIST", Role::Special }, { "specialist", Role::Special },
        { "Specialist", Role::Special },

        { "drone", Role::Drone },        { "DRONE", Role::Drone },        { "Drone", Role::Drone },
        { "无人机", Role::Drone },       { "SUMMON", Role::Drone },       { "summon", Role::Drone },
        { "Summon", Role::Drone },       { "召唤物", Role::Drone },
    };
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
    bool is_unusual_location = false; // 地面辅助，高台先锋等
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
using OperUsageGroups = std::unordered_map<std::string, std::vector<OperUsage>>;

enum class ActionType
{
    Deploy,      // 部署干员
    UseSkill,    // 开技能
    Retreat,     // 撤退干员
    SkillUsage,  // 技能用法
    SwitchSpeed, // 切换二倍速
    BulletTime,  // 使用 1/5 的速度
    Output,      // 仅输出，什么都不操作，界面上也不显示
    SkillDaemon, // 什么都不做，有技能开技能，直到战斗结束

    /* for TRN */
    MoveCamera, // 引航者试炼，移动镜头

    /* for SSS */
    DrawCard,         // “调配干员”
    CheckIfStartOver, // 检查如果没有某干员则退出重开

    /* for Enhance */
    Loop,       // 创建一个循环流程
    Case,       // 为干员组进行分化操作
    Check,      // 根据触发条件执行不同的分支
    Until,      // 内部动作可无序，知道里边的动作全部执行完毕才能结束
    SavePoint,  // 保存一个锚点
    SyncPoint,  // 同步锚点
    CheckPoint, // 检查锚点
};

struct Action;
using ActionPtr = std::shared_ptr<Action>;

struct TriggerInfo
{
    static constexpr int DEACTIVE_KILLS = -1;       // 击杀数未被设置
    static constexpr int DEACTIVE_COST = -1;        // 费用数未被设置
    static constexpr int DEACTIVE_COST_CHANGES = 0; // 费用变更未设置
    static constexpr int DEACTIVE_COOLING = -1;     // 干员冷却数未设置
    static constexpr int DEACTIVE_COUNT = -1;       // 循环计数

    enum class Category
    {
        None, // 未设置, 未指定策略
        Succ, // 默认满足, 跳过条件判断
        All,  // 所有被设置的条件都应该满足
        Any,  // 所有被设置的条件只要有一个满足
        Not   // 所有被设置的条件一个都不满足
    };

    Category category = Category::None;

    int kills = DEACTIVE_KILLS;               // 击杀数条件
    int costs = DEACTIVE_COST;                // 费用条件
    int cost_changes = DEACTIVE_COST_CHANGES; // 费用变化条件
    int cooling = DEACTIVE_COOLING;           // 冷却中的干员条件
    int count = DEACTIVE_COUNT;               // 计数条件，不做变化，记录初始值

    mutable int counter = 0;                  // 计数器

    // 触发器是否被激活
    bool active() const noexcept { return category != Category::None; }

    // 重置计数器
    void resetCounter() const noexcept { counter = 0; }

    // 激活一次计数器
    void activeCounter() const noexcept { ++counter; }

    static auto loadCategoryFrom(std::string const& _Str) -> Category
    {
        if (_Str == "succ") {
            return Category::Succ;
        }

        if (_Str == "all") {
            return Category::All;
        }

        if (_Str == "any") {
            return Category::Any;
        }

        if (_Str == "not") {
            return Category::Not;
        }

        return Category::None;
    }
};

// 用于定义延时信息，每个命令都有
struct DelayInfo
{
    int pre_delay = 0;      // 执行动作前的延时
    int post_delay = 0;     // 执行动作之后的延时
    int time_out = INT_MAX; // TODO
};

// 定义干员操作信息
struct AvatarInfo
{
    std::string name;                                   // 目标名，若 type >= SwitchSpeed, name 为空
    Point location;                                     // 目标定位
    DeployDirection direction = DeployDirection::Right; // 目标朝向
    SkillUsage modify_usage = SkillUsage::NotUse;       // 目标的技能使用策略
    int modify_times = 1; // 更改使用技能的次数，默认为 1，兼容曾经的作业
};

// 定义文本说明信息
struct TextInfo
{
    std::string doc;       // 文本
    std::string doc_color; // 文本颜色
};

// 定义case操作需要的信息
struct CaseInfo
{
    std::string group_select;                                       // 选定的干员组
    std::map<std::string, std::vector<ActionPtr>> dispatch_actions; // 对应干员的动作
    std::vector<ActionPtr> default_action;                          // 默认的动作
};

// 定义loop操作需要的信息
struct LoopInfo
{
    TriggerInfo end_info;
    TriggerInfo continue_info;
    TriggerInfo break_info;

    int counter = 0; // 计数器

    std::vector<ActionPtr> loop_actions;
};

// 定义check操作需要的信息
struct CheckInfo
{
    TriggerInfo condition_info;
    std::vector<ActionPtr> then_actions; // 当条件满足时执行
    std::vector<ActionPtr> else_actions; // 当条件不满足时执行
};

// 定义until操作需要的信息
struct UntilInfo
{
    TriggerInfo::Category mode;       // all 全部命令执行完毕后才结束， any 只要有一个执行就结束
    std::vector<ActionPtr> candidate; // 备用的命令序列
};

// 定义point操作需要的信息
struct PointInfo
{
    struct SnapShot
    {
        int kills = TriggerInfo::DEACTIVE_KILLS;
        int cost = TriggerInfo::DEACTIVE_COST;
        int64_t cooling_count = TriggerInfo::DEACTIVE_COOLING;
        int interval = 0;

        std::chrono::steady_clock::time_point tNow;
    };

    std::string target_code;
    TriggerInfo::Category mode = TriggerInfo::Category::All;
    std::pair<SnapShot, SnapShot> range;

    std::vector<ActionPtr> then_actions; // 当条件满足时执行
    std::vector<ActionPtr> else_actions; // 当条件不满足时执行
};

struct CheckIfStartOverInfo
{
    std::string name;
    RoleCounts role_counts; // 角色统计

    CheckIfStartOverInfo() = default;

    explicit CheckIfStartOverInfo(RoleCounts&& _Counts) :
        role_counts(std::move(_Counts))
    {
    }
};

struct MoveCameraInfo
{
    std::pair<double, double> distance;

    explicit MoveCameraInfo(decltype(distance)&& _Dist) :
        distance(std::move(_Dist))
    {
    }
};

struct Action
{
    ActionType type = ActionType::Deploy;
    std::string point_code; // 锚点编码
    TriggerInfo trigger;    // 必须拥有，表示动作触发时的条件信息
    DelayInfo delay;        // 延时信息
    TextInfo text;          //  表示文本输出，用于命令提示亦或是调试输出

    // 为了便于内存管理，使用variant管理额外的action信息，通过type来进行还原
    // AvatarInfo 表示干员或辅助装置的部署、撤退、技能策略更改等
    // CaseInfo 表示对干员组内选择对不同干员进行分化动作
    // LoopInfo 表示循环命令中对动作信息的相关设置
    // CheckInfo  表示根据条件的成立于否来确认执行的命令序列
    // UntilInfo  表示根据条件的成立于否来无序执行命令序列
    std::variant<
        std::monostate,
        CheckIfStartOverInfo,
        MoveCameraInfo,
        AvatarInfo,
        CaseInfo,
        LoopInfo,
        CheckInfo,
        UntilInfo,
        PointInfo>
        payload; // 信息载荷

    // 是否携带干员信息
    bool hasAvatarInfo() const noexcept { return std::holds_alternative<AvatarInfo>(payload); }

    // 读取载荷
    template <typename PayLoad_T>
    auto getPayload() const noexcept -> PayLoad_T const&
    {
        return std::get<PayLoad_T>(payload);
    }

    // 创建堆对象，由智能指针管理内存
    static auto create() -> ActionPtr { return std::make_shared<Action>(); }
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
    std::string core;
    RoleCounts tool_men;
    Point location;
    DeployDirection direction = DeployDirection::None;
};

struct CombatData : public copilot::CombatData
{
    std::vector<Strategy> strategies;
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
