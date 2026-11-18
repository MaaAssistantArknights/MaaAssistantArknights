#include "ReclamationTask.h"

#include <climits>

#include "Task/ProcessTask.h"

// 通用配置及插件
#include "Task/Reclamation/ReclamationConfig.h"

// TalesMode::ProsperityInSave 专用配置及插件
#include "Task/Reclamation/ReclamationCraftTaskPlugin.h"

#include "Utils/Logger.hpp"

asst::ReclamationTask::ReclamationTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_reclamation_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType)),
    m_config_ptr(std::make_shared<ReclamationConfig>())
{
    LogTraceFunction;

    // TalesMode::ProsperityInSave 专用参数
    m_reclamation_task_ptr->register_plugin<ReclamationCraftTaskPlugin>(m_config_ptr);

    m_subtasks.emplace_back(m_reclamation_task_ptr);
}

bool asst::ReclamationTask::set_params(const json::value& params)
{
    LogTraceFunction;

    return m_config_ptr->verify_and_load_params(params);
}

bool asst::ReclamationTask::run()
{
    LogTraceFunction;

    // 在工作线程选择任务入口，避免更新参数时改写正在执行的 ProcessTask。
    const auto params = m_config_ptr->get_params();
    const auto& theme = params->theme;
    const auto& mode = params->mode;

    if (theme == ReclamationTheme::Fire) {
        Log.info(__FUNCTION__, "Reclamation Algorithm theme", theme, "is no longer available");
        m_reclamation_task_ptr->set_tasks({ "Stop" });
        return InterfaceTask::run();
    }

    m_reclamation_task_ptr->set_times_limit("RA@Store@EnterStore", INT_MAX);

    if (const auto* ra_mode = std::get_if<RelaunchAnchorMode>(&mode)) {
        // 重启锚点：根据 mode 选择关卡入口
        const int stage = static_cast<int>(*ra_mode);
        std::string stage_str;
        switch (stage) {
        case static_cast<int>(RelaunchAnchorMode::RA1):
            stage_str = "1";
            break;
        case static_cast<int>(RelaunchAnchorMode::RA4):
            stage_str = "4";
            break;
        case static_cast<int>(RelaunchAnchorMode::RA15):
            stage_str = "15";
            break;
        default:
            stage_str = "1";
            break;
        }
        const std::string task_name = theme + "@RA@RA" + stage_str + "-Entry";
        m_reclamation_task_ptr->set_tasks({ task_name });
        m_reclamation_task_ptr->set_times_limit("RA@Store@EnterStore", 0);
    }
    else if (const auto* tales_mode = std::get_if<TalesMode>(&mode)) {
        switch (*tales_mode) {
        case TalesMode::ProsperityNoSave:
            m_reclamation_task_ptr->set_tasks({ theme + "@RA@ProsperityNoSave" });
            if (!params->clear_store) {
                m_reclamation_task_ptr->set_times_limit("RA@Store@EnterStore", 0);
            }
            break;
        case TalesMode::ProsperityInSave:
            m_reclamation_task_ptr->set_tasks({ theme + "@RA@ProsperityInSave" });
            break;
        default:
            break;
        }
    }

    for (const auto& plugin : m_reclamation_task_ptr->get_plugins()) {
        if (const auto reclamation_plugin = std::dynamic_pointer_cast<AbstractReclamationTaskPlugin>(plugin)) {
            reclamation_plugin->set_enable(reclamation_plugin->load_params(*params));
        }
    }
    return InterfaceTask::run();
}
