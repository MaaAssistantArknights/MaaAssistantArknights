#include "SeasonalTask.h"

#include "Task/ProcessTask.h"
#include "Task/Stronghold/StrongholdProtocolTaskPlugin.h"
#include "Utils/Logger.hpp"

asst::SeasonalTask::SeasonalTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    LogTraceFunction;

    m_plugin_ptr = m_task_ptr->register_plugin<StrongholdProtocolTaskPlugin>();
    m_plugin_ptr->set_covenant(true);
    m_task_ptr->set_tasks({ "StrongholdProtocol@Farm@Begin" });
    m_subtasks.emplace_back(m_task_ptr);
}

bool asst::SeasonalTask::set_params(const json::value& params)
{
    LogTraceFunction;

    const int rounds = params.get("rounds", 3);
    if (rounds <= 0) {
        // 0 = 跳过任务（GUI 侧已记日志），空跑结束
        LogInfo << __FUNCTION__ << "rounds" << rounds << "is not positive, skip task";
        m_task_ptr->set_tasks({ "Stop" });
        return true;
    }

    // 核心盟约：大盟约（阵营盟约）与小盟约（非阵营盟约）任一命中的干员按核心优先买入部署，空 = 不启用
    const std::string major = params.get("core_major_bond", std::string());
    const std::string minor = params.get("core_minor_bond", std::string());

    // 每回合升级计划：数组下标 = 回合-1，值为该回合目标商店等级，0 = 本回合不升级
    std::vector<int> upgrade_plan;
    if (const auto plan = params.find("upgrade_plan"); plan && plan->is_array()) {
        for (const auto& item : plan->as_array()) {
            if (!item.is_number() || item.as_integer() < 0 || item.as_integer() > 6) {
                LogError << __FUNCTION__ << "invalid upgrade_plan entry:" << item;
                return false;
            }
            upgrade_plan.emplace_back(item.as_integer());
        }
    }

    m_task_ptr->set_tasks({ "StrongholdProtocol@Farm@Begin" });
    m_plugin_ptr->set_core_bond(major, minor).set_upgrade_plan(std::move(upgrade_plan)).set_runs(rounds);
    return true;
}
