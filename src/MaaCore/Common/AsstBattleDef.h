#pragma once

#include <string>
#include <unordered_map>
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
        NotUse = 0,   // 不自动使用
        Possibly = 1, // 有就用，例如干员 棘刺 3 技能
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
            { "Summon", Role::Drone },       { "召唤物", Role::Drone },       { "CEMENT", Role::Drone },
            { "cement", Role::Drone },       { "Cement", Role::Drone },       { "超重绝缘水泥", Role::Drone },
            { "水泥", Role::Drone },
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
            // 以下为非json解析字段
            int index;
            bool core_deployed = false; // 全局保core，再次识别到core则同location全部重置
            bool operator==(const Strategy& other) const
            {
                return index == other.index;
            }
        };

        using StrategyOrderLock = std::unordered_map<Point, std::vector<std::shared_ptr<Strategy>>>;

        struct CombatData : public copilot::CombatData
        {
            std::vector<Strategy> strategies;
            bool draw_as_possible = false;
            int retry_times = 0;
            std::vector<std::string> order_of_drops;
            StrategyOrderLock order;
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
