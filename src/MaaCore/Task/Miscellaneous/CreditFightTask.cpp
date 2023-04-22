#include "CreditFightTask.h"

#include <utility>

#include "Config/TaskData.h"

#include "Task/AbstractTask.h"
#include "Task/Fight/StageNavigationTask.h"
#include "Task/Interface/CopilotTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Ranges.hpp"
#include "Utils/WorkingDir.hpp"

asst::CreditFightTask::CreditFightTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain)
    : PackageTask(callback, inst, task_chain)
{
    auto start_up_task_ptr = std::make_shared<ProcessTask>(m_callback, m_inst, task_chain);
    auto stage_navigation_task_ptr = std::make_shared<StageNavigationTask>(m_callback, m_inst, task_chain);
    auto stop_task_ptr = std::make_shared<ProcessTask>(m_callback, m_inst, task_chain);
    auto not_use_prts_task_ptr = std::make_shared<ProcessTask>(m_callback, m_inst, task_chain);
    auto copilot_task_ptr = std::make_shared<CopilotTask>(m_callback, m_inst);

    // 开始
    start_up_task_ptr->set_tasks({ "StageBegin" }).set_times_limit("GoLastBattle", 0);

    // 自动战斗
    json::value copilot_params = json::object {
        { "filename", utils::path_to_utf8_string(ResDir.get() / "copilot" / "OF-1_credit_fight.json") },
        { "formation", true },
        { "support_unit_name", "_RANDOM_" },
    };
    copilot_task_ptr->set_params(copilot_params);

    // 关卡导航
    stage_navigation_task_ptr->set_stage_name(copilot_task_ptr->get_stage_name());

    // 不使用代理指挥
    not_use_prts_task_ptr->set_tasks({ "NotUsePrts", "Stop" });

    // 战斗结束后
    stop_task_ptr->set_tasks({ "EndOfActionThenStop", "FightMissionFailedThenStop" });

    m_subtasks.emplace_back(start_up_task_ptr);
    m_subtasks.emplace_back(stage_navigation_task_ptr);
    m_subtasks.emplace_back(not_use_prts_task_ptr);
    m_subtasks.emplace_back(copilot_task_ptr);
    m_subtasks.emplace_back(stop_task_ptr);
}
