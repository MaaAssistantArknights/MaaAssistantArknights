#include "CopilotTask.h"

#include <regex>

#include "Config/Miscellaneous/CopilotConfig.h"
#include "Task/Miscellaneous/BattleFormationTask.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

asst::CopilotTask::CopilotTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      m_formation_task_ptr(std::make_shared<BattleFormationTask>(callback, inst, TaskType)),
      m_battle_task_ptr(std::make_shared<BattleProcessTask>(callback, inst, TaskType)),
      m_stop_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    LogTraceFunction;

    auto start_1_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_1_tp->set_tasks({ "BattleStartPre" }).set_retry_times(0).set_ignore_error(true);
    m_subtasks.emplace_back(start_1_tp);

    m_subtasks.emplace_back(m_formation_task_ptr);

    auto start_2_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_2_tp->set_tasks({ "BattleStartAll" }).set_ignore_error(false);
    m_subtasks.emplace_back(start_2_tp);

    m_subtasks.emplace_back(m_battle_task_ptr)->set_retry_times(0);

    m_stop_task_ptr->set_tasks({ "ClickCornerUntilStartButton" });
    m_stop_task_ptr->set_enable(false);
    m_subtasks.emplace_back(m_stop_task_ptr);
}

bool asst::CopilotTask::set_params(const json::value& params)
{
    LogTraceFunction;

    if (m_running) {
        return false;
    }

    auto filename_opt = params.find<std::string>("filename");
    if (!filename_opt) {
        Log.error("CopilotTask set_params failed, stage_name or filename not found");
        return false;
    }

    static const std::regex maa_regex(R"(^maa://(\d+)$)");
    std::smatch match;

    if (std::regex_match(*filename_opt, match, maa_regex)) {
        if (!Copilot.parse_magic_code(match[1].str())) {
            Log.error("CopilotConfig parse failed");
            return false;
        }
    }
    else if (!Copilot.load(utils::path(*filename_opt))) {
        Log.error("CopilotConfig parse failed");
        return false;
    }

    m_stage_name = Copilot.get_stage_name();
    if (!m_battle_task_ptr->set_stage_name(m_stage_name)) {
        Log.error("Not support stage");
        return false;
    }

    bool with_formation = params.get("formation", false);
    m_formation_task_ptr->set_enable(with_formation);
    std::string support_unit_name = params.get("support_unit_name", std::string());
    m_formation_task_ptr->set_support_unit_name(std::move(support_unit_name));

    size_t loop_times = params.get("loop_times", 1);
    if (loop_times > 1) {
        m_subtasks.reserve(m_subtasks.size() * loop_times);
        auto raw_end = m_subtasks.end();
        for (size_t i = 1; i < loop_times; ++i) {
            // FIXME: 如果多次调用 set_params，这里复制的会有问题
            m_subtasks.insert(m_subtasks.end(), m_subtasks.begin(), raw_end);
        }
        m_stop_task_ptr->set_enable(true);
    }
    else {
        m_stop_task_ptr->set_enable(false);
    }

    return true;
}
