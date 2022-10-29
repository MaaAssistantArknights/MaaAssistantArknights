#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Utils/NoWarningCVMat.h"

#include "Utils/AsstTypes.h"

namespace asst
{
    enum class BattleSkillUsage // 干员技能使用方法
    {
        NotUse = 0,   // 不自动使用
        Possibly = 1, // 有就用，例如干员 棘刺 3 技能
        Once = 2,     // 只用一次，例如干员 山 2 技能
        InTime = 3,   // 自动判断使用时机，画饼.jpg
        OnceUsed
    };
    struct BattleDeployOper // 干员
    {
        std::string name;
        int skill = 1; // 技能序号，取值范围 [1, 3]
        BattleSkillUsage skill_usage = BattleSkillUsage::NotUse;
    };
    struct BattleDeployInfo
    {
        Point loc;
        Point pos;
        BattleDeployOper info;
    };

    enum class BattleDeployDirection
    {
        Right = 0,
        Down = 1,
        Left = 2,
        Up = 3,
        None = 4 // 没有方向，通常是无人机之类的
    };

    enum class BattleActionType // 操作类型
    {
        Deploy,      // 部署干员
        UseSkill,    // 开技能
        Retreat,     // 撤退干员
        SkillUsage,  // 技能用法
        SwitchSpeed, // 切换二倍速
        BulletTime,  // 使用 1/5 的速度（点击任意干员），会在下一个任意操作后恢复原速度
        UseAllSkill, // 使用所有技能，仅肉鸽模式
        Output,      // 仅输出，什么都不操作，界面上也不显示
        SkillDaemon, // 什么都不做，有技能开技能，直到战斗结束
    };

    struct BattleAction // 操作
    {
        int kills = 0;
        int cost_changes = 0;
        int cooling = 0;
        BattleActionType type = BattleActionType::Deploy;
        std::string group_name; // 目标名，若 type >= SwitchSpeed, group_name 为空
        Point location;
        BattleDeployDirection direction = BattleDeployDirection::Right;
        BattleSkillUsage modify_usage = BattleSkillUsage::NotUse;
        int pre_delay = 0;
        int rear_delay = 0;
        int time_out = INT_MAX;
        std::string doc;
        std::string doc_color;
    };

    struct BattleCopilotData
    {
        std::string minimum_required;
        std::string title;
        std::string title_color;
        std::string details;
        std::string details_color;
        std::unordered_map<std::string, std::vector<BattleDeployOper>> groups;
        std::vector<BattleAction> actions;
    };

    enum class BattleRole
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

    inline BattleRole get_role_type(const std::string& role_name)
    {
        static const std::unordered_map<std::string, BattleRole> NameToRole = {
            { "warrior", BattleRole::Warrior }, { "WARRIOR", BattleRole::Warrior },
            { "Warrior", BattleRole::Warrior }, { "近卫", BattleRole::Warrior },

            { "pioneer", BattleRole::Pioneer }, { "PIONEER", BattleRole::Pioneer },
            { "Pioneer", BattleRole::Pioneer }, { "先锋", BattleRole::Pioneer },

            { "medic", BattleRole::Medic },     { "MEDIC", BattleRole::Medic },
            { "Medic", BattleRole::Medic },     { "医疗", BattleRole::Medic },

            { "tank", BattleRole::Tank },       { "TANK", BattleRole::Tank },
            { "Tank", BattleRole::Tank },       { "重装", BattleRole::Tank },

            { "sniper", BattleRole::Sniper },   { "SNIPER", BattleRole::Sniper },
            { "Sniper", BattleRole::Sniper },   { "狙击", BattleRole::Sniper },

            { "caster", BattleRole::Caster },   { "CASTER", BattleRole::Caster },
            { "Caster", BattleRole::Caster },   { "术师", BattleRole::Caster },

            { "support", BattleRole::Support }, { "SUPPORT", BattleRole::Support },
            { "Support", BattleRole::Support }, { "辅助", BattleRole::Support },

            { "special", BattleRole::Special }, { "SPECIAL", BattleRole::Special },
            { "Special", BattleRole::Special }, { "特种", BattleRole::Special },

            { "drone", BattleRole::Drone },     { "DRONE", BattleRole::Drone },
            { "Drone", BattleRole::Drone },     { "无人机", BattleRole::Drone },
        };
        if (auto iter = NameToRole.find(role_name); iter != NameToRole.end()) {
            return iter->second;
        }
        return BattleRole::Unknown;
    }
} // namespace asst

namespace std
{
    inline std::string to_string(const asst::BattleRole& role)
    {
        static const std::unordered_map<asst::BattleRole, std::string> RoleToName = {
            { asst::BattleRole::Warrior, "Warrior" }, { asst::BattleRole::Pioneer, "Pioneer" },
            { asst::BattleRole::Medic, "Medic" },     { asst::BattleRole::Tank, "Tank" },
            { asst::BattleRole::Sniper, "Sniper" },   { asst::BattleRole::Caster, "Caster" },
            { asst::BattleRole::Support, "Support" }, { asst::BattleRole::Special, "Special" },
            { asst::BattleRole::Drone, "Drone" },     { asst::BattleRole::Unknown, "Unknown" }
        };
        return RoleToName.at(role);
    }
}

namespace asst
{
    struct BattleRealTimeOper
    {
        int cost = 0;
        BattleRole role = BattleRole::Unknown;
        bool available = false;
        Rect rect;
        cv::Mat avatar;
        std::string name;
        size_t index = 0;
        bool cooling = false;
    };

    struct ReplacementHome
    {
        Point location;
        BattleDeployDirection direction;
    };

    struct ForceDeployDirection
    {
        BattleDeployDirection direction;
        std::unordered_set<BattleRole> role;
    };

    struct RoguelikeBattleData
    {
        std::string stage_name;
        std::vector<ReplacementHome> replacement_home;
        std::unordered_set<Point> blacklist_location;
        std::unordered_map<Point, ForceDeployDirection> force_deploy_direction;
        std::vector<int> key_kills;
        std::array<BattleRole, 9> role_order;
        bool use_dice_stage = true;
        int stop_deploy_blocking_num = INT_MAX;
        int force_deploy_air_defense_num = 0;
        bool force_ban_medic = false;
    };

    enum class BattleOperPosition
    {
        None,
        Blocking,   // 阻挡单位
        AirDefense, // 对空单位
    };

    enum class BattleLocationType
    {
        Invalid = -1,
        None = 0,
        Melee = 1,
        Ranged = 2,
        All = 3
    };

    struct BattleCharData
    {
        std::string name;
        BattleRole role;
        std::array<std::string, 3> ranges;
        int rarity = 0;
        bool with_direction = true;
        BattleLocationType location_type = BattleLocationType::None;
    };

    struct BattleRecruitOperInfo
    {
        std::string name;
        Rect rect;
        int elite = 0;
        int level = 0;
    };

    using BattleAttackRange = std::vector<Point>;
} // namespace asst
