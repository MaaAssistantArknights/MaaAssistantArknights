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
        Possibly = 0,               // 有就用，例如干员 棘刺 3 技能
        Once = 1,                   // 只用一次，例如干员 银灰 2技能
        InTime = 2,                 // 关键时刻使用，自动判断使用时机，例如干员 假日威龙陈 3 技能
        NotUse = 3,                 // 不自动使用
        OnceUsed
    };
    struct BattleDeployOper         // 干员
    {
        std::string name;
        int skill = 1;              // 技能序号，取值范围 [1, 3]
        BattleSkillUsage skill_usage = BattleSkillUsage::Possibly;
    };

    enum class BattleDeployDirection
    {
        Right = 0,
        Down = 1,
        Left = 2,
        Up = 3,
        No = 4                      // 没有方向，通常是无人机之类的
    };

    enum class BattleActionType     // 操作类型
    {
        Deploy = 0,                 // 部署干员
        Retreat = 1,                // 撤退干员
        UseSkill = 2,               // 开技能
        SwitchSpeed = 100,          // 切换二倍速
        SlowMode = 101              // 使用 1/5 的速度（点击任意干员），会在下一个任意操作后恢复原速度
    };

    struct BattleAction             // 操作
    {
        int kills_condition = 0;
        BattleActionType type = BattleActionType::Deploy;
        std::string group_name;     // 目标名，若 type >= SwitchSpeed, group_name 为空
        Point location;
        BattleDeployDirection direction = BattleDeployDirection::Right;
        int pre_delay = 0;
        int rear_delay = 0;
        int time_out = INT_MAX;
    };

    struct BattleActionsGroup
    {
        std::unordered_map<std::string, std::vector<BattleDeployOper>> opers_groups;
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
    };
}
