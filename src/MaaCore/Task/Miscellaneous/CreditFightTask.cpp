#include "CreditFightTask.h"

#include "Config/TaskData.h"

#include "Task/AbstractTask.h"
#include "Task/ProcessTask.h"
#include <ranges>

asst::CreditFightTask::CreditFightTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain) :
    PackageTask(callback, inst, task_chain),
    m_stage_navigation_task_ptr(std::make_shared<StageNavigationTask>(m_callback, m_inst, task_chain)),
    m_copilot_task_ptr(std::make_shared<CopilotTask>(m_callback, m_inst))
{
    auto start_up_task_ptr = std::make_shared<ProcessTask>(m_callback, m_inst, task_chain);
    auto stop_task_ptr = std::make_shared<ProcessTask>(m_callback, m_inst, task_chain);
    auto not_use_prts_task_ptr = std::make_shared<ProcessTask>(m_callback, m_inst, task_chain);

    // 开始
    start_up_task_ptr->set_tasks({ "StageBegin" }).set_times_limit("GoLastBattle", 0);

    // 不使用代理指挥
    not_use_prts_task_ptr->set_tasks({ "NotUsePrts", "Stop" });

    // 战斗结束后
    stop_task_ptr->set_tasks({ "EndOfActionThenStop", "FightMissionFailedThenStop" });

    m_subtasks.emplace_back(start_up_task_ptr);
    m_subtasks.emplace_back(m_stage_navigation_task_ptr);
    m_subtasks.emplace_back(not_use_prts_task_ptr);
    m_subtasks.emplace_back(m_copilot_task_ptr);
    m_subtasks.emplace_back(stop_task_ptr);
}

bool asst::CreditFightTask::set_params(int formation_index)
{
    // 自动战斗
    json::value copilot_params = json::object {
        { "filename", utils::path_to_utf8_string(ResDir.get() / "copilot" / "OF-1_credit_fight.json") },
        { "formation", true },
        { "support_unit_usage", 3 },
        { "formation_index", formation_index },
    };
    return m_copilot_task_ptr->set_params(copilot_params) &&
           m_stage_navigation_task_ptr->set_stage_name(m_copilot_task_ptr->get_stage_name());
}
