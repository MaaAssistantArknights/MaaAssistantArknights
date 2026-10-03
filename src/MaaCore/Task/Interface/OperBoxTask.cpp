#include "OperBoxTask.h"

#include "Task/Miscellaneous/OperBoxRecognitionTask.h"
#include "Task/Miscellaneous/ScreenshotTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

asst::OperBoxTask::OperBoxTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType)
{
}

bool asst::OperBoxTask::set_params(const json::value& params)
{
    LogTraceFunction;

    m_paradox_filter = params.get("paradox_filter", false);
    m_next_only = params.get("paradox_next", false);
    m_candidates = params.get("paradox_candidates", std::vector<std::string> {});
    build_subtasks();
    m_subtasks_built = true;
    return true;
}

bool asst::OperBoxTask::run()
{
    LogTraceFunction;
    if (!m_subtasks_built) {
        build_subtasks();
        m_subtasks_built = true;
    }
    return InterfaceTask::run();
}

void asst::OperBoxTask::build_subtasks()
{
    LogTraceFunction;
    m_subtasks.clear();

    if (m_paradox_filter) {
        auto recognition = std::make_shared<OperBoxRecognitionTask>(m_callback, m_inst, TaskType);
        recognition->set_paradox_filter(true);
        recognition->set_next_only(m_next_only, m_candidates);
        recognition->set_retry_times(0);
        m_subtasks.emplace_back(std::move(recognition));
        return;
    }

    auto enter_task = std::make_shared<ProcessTask>(m_callback, m_inst, TaskType);
    enter_task->set_tasks({ "OperBoxBegin" }).set_ignore_error(true);
    enter_task->register_plugin<ScreenshotTaskPlugin>();
    m_subtasks.emplace_back(std::move(enter_task));

    auto recognition = std::make_shared<OperBoxRecognitionTask>(m_callback, m_inst, TaskType);
    recognition->set_retry_times(0);
    m_subtasks.emplace_back(std::move(recognition));
}
