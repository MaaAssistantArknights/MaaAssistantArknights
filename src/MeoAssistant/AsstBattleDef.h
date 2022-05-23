#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include <opencv2/core/mat.hpp>

#include "AsstTypes.h"

namespace asst
{
    enum class BattleSkillUsage     // 干员技能使用方法
    {
        NotUse = 0,                 // 不自动使用
        Possibly = 1,               // 有就用，例如干员 棘刺 3 技能
        Once = 2,                   // 只用一次，例如干员 山 2 技能
        InTime = 3,                 // 自动判断使用时机，画饼.jpg
        OnceUsed
    };
    struct BattleDeployOper         // 干员
    {
        std::string name;
        int skill = 1;              // 技能序号，取值范围 [1, 3]
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
        None = 4                      // 没有方向，通常是无人机之类的
    };

    enum class BattleActionType     // 操作类型
    {
        Deploy,                     // 部署干员
        UseSkill,                   // 开技能
        Retreat,                    // 撤退干员
        SkillUsage,                 // 技能用法
        SwitchSpeed,                // 切换二倍速
        BulletTime,                 // 使用 1/5 的速度（点击任意干员），会在下一个任意操作后恢复原速度
        UseAllSkill                 // 使用所有技能，仅肉鸽模式
    };

    struct BattleAction             // 操作
    {
        int kills = 0;
        BattleActionType type = BattleActionType::Deploy;
        std::string group_name;     // 目标名，若 type >= SwitchSpeed, group_name 为空
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

    struct BattleRealTimeOper
    {
        int cost = 0;
        BattleRole role = BattleRole::Unknown;
        bool available = false;
        Rect rect;
        cv::Mat avatar;
        std::string name;
        size_t index;
        bool cooling = false;
        bool operator==(const std::string& oper_name) const noexcept
        {
            return name == oper_name;
        }
    };

    struct RoguelikeBattleAction
    {
        int kills = 0;
        BattleActionType type = BattleActionType::Deploy;
        std::vector<BattleRole> roles;
        bool waiting_cost = false;
        Point location;
        BattleDeployDirection direction = BattleDeployDirection::Right;
        int pre_delay = 0;
        int rear_delay = 0;
        int time_out = INT_MAX;
    };
}
