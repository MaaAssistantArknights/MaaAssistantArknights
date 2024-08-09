#include "SSSCopilotTask.h"

#include "Config/Miscellaneous/SSSCopilotConfig.h"
#include "Config/TaskData.h"
#include "Task/Miscellaneous/BattleFormationTask.h"
#include "Task/ProcessTask.h"
#include "Task/SSS/SSSDropRewardsTaskPlugin.h"
#include "Task/SSS/SSSStageManagerTask.h"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

asst::SSSCopilotTask::SSSCopilotTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_begin_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType)),
    m_formation_task_ptr(std::make_shared<BattleFormationTask>(callback, inst, TaskType)),
    m_stage_task_ptr(std::make_shared<SSSStageManagerTask>(callback, inst, TaskType))
{
    LogTraceFunction;

    m_begin_task_ptr->set_tasks({ inst_string() + "@SSSBegin", "SSSStartFighting" })
        .set_times_limit("SSSStartFighting", 0)
        .set_ignore_error(false);
    m_subtasks.emplace_back(m_begin_task_ptr);

    m_formation_task_ptr->set_data_resource(BattleFormationTask::DataResource::SSSCopilot);
    m_subtasks.emplace_back(m_formation_task_ptr);

    auto start_2_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_2_tp->set_tasks({ inst_string() + "@SSSTeamConfirm", "SSSStartFighting" })
        .set_times_limit("SSSStartFighting", 0)
        .set_ignore_error(false);
    m_subtasks.emplace_back(start_2_tp);

    m_stage_task_ptr->register_plugin<SSSDropRewardsTaskPlugin>();
    m_subtasks.emplace_back(m_stage_task_ptr);
}

bool asst::SSSCopilotTask::set_params(const json::value& params)
{
    LogTraceFunction;

    if (m_running) {
        Log.error("SSSCopilotTask not support set_params when running");
        return false;
    }

    auto filename_opt = params.find<std::string>("filename");
    if (!filename_opt) {
        Log.error("SSSCopilotTask set_params failed, stage_name or filename not found");
        return false;
    }

    if (!SSSCopilot.load(utils::path(*filename_opt))) {
        Log.error("SSSCopilotConfig parse failed");
        return false;
    }

    if (const auto& buff = SSSCopilot.get_data().buff; !buff.empty()) {
        Task.get<OcrTaskInfo>(inst_string() + "@SSSBuffChoose")->text = { buff };
    }

    // bool with_formation = params.get("formation", false);
    // m_formation_task_ptr->set_enable(with_formation);

    // std::string support_unit_name = params.get("support_unit_name", std::string());
    // m_formation_task_ptr->set_support_unit_name(std::move(support_unit_name));

    // BattleFormationTask::AdditionalFormation additional_formation {
    //     .filter = BattleFormationTask::Filter::Cost,
    //     .double_click_filter = true,
    //     .role_counts = SSSCopilot.get_data().tool_men,
    // };
    // m_formation_task_ptr->append_additional_formation(std::move(additional_formation));

    // 暂时不支持自动编队
    m_formation_task_ptr->set_enable(false);

    size_t loop_times = params.get("loop_times", 1);
    if (loop_times > 1) {
        m_subtasks.reserve(m_subtasks.size() * loop_times);
        auto raw_end = m_subtasks.end();
        for (size_t i = 1; i < loop_times; ++i) {
            // FIXME: 如果多次调用 set_params，这里复制的会有问题
            m_subtasks.insert(m_subtasks.end(), m_subtasks.begin(), raw_end);
        }
    }

    return true;
}
