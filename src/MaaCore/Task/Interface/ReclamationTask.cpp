#include "ReclamationTask.h"

#include <climits>

#include "Task/ProcessTask.h"

// 通用配置及插件
#include "Task/Reclamation/ReclamationConfig.h"

// 沙洲遗闻的模式插件
#include "Task/Reclamation/ReclamationCraftTaskPlugin.h"
#include "Task/Reclamation/ReclamationResourceTaskPlugin.h"

#include "Utils/Logger.hpp"

asst::ReclamationTask::ReclamationTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_reclamation_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType)),
    m_config_ptr(std::make_shared<ReclamationConfig>())
{
    LogTraceFunction;

    // 沙洲遗闻的模式插件
    m_reclamation_task_ptr->register_plugin<ReclamationCraftTaskPlugin>(m_config_ptr);
    m_reclamation_task_ptr->register_plugin<ReclamationResourceTaskPlugin>(m_config_ptr);

    m_subtasks.emplace_back(m_reclamation_task_ptr);
}

bool asst::ReclamationTask::set_params(const json::value& params)
{
    LogTraceFunction;

    if (!m_config_ptr->verify_and_load_params(params)) {
        return false;
    }

    const std::string& theme = m_config_ptr->get_theme();
    const auto& mode = m_config_ptr->get_mode();

    if (theme == ReclamationTheme::Fire) {
        Log.info(__FUNCTION__, "Reclamation Algorithm theme", theme, "is no longer available");
        m_reclamation_task_ptr->set_tasks({ "Stop" });
        return true;
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
            if (!params.get("clear_store", false)) {
                m_reclamation_task_ptr->set_times_limit("RA@Store@EnterStore", 0);
            }
            break;
        case TalesMode::ProsperityInSave:
            m_reclamation_task_ptr->set_tasks({ theme + "@RA@ProsperityInSave" });
            break;
        case TalesMode::ResourceInSave:
            m_reclamation_task_ptr->set_tasks({ theme + "@RA@ResourceInSave" });
            m_reclamation_task_ptr->set_times_limit("RA@Store@EnterStore", 0);
            break;
        default:
            break;
        }
    }

    // 各生息演算插件根据 params 设置插件专用参数, 停用不应被启用的插件
    for (const TaskPluginPtr& plugin : m_reclamation_task_ptr->get_plugins()) {
        if (const auto& reclamation_task_plugin = std::dynamic_pointer_cast<AbstractReclamationTaskPlugin>(plugin);
            reclamation_task_plugin != nullptr) {
            reclamation_task_plugin->set_enable(reclamation_task_plugin->load_params(params));
        }
    }

    return true;
}

bool asst::ReclamationTask::run()
{
    if (!get_enable()) {
        return InterfaceTask::run();
    }
    const auto plugin = m_reclamation_task_ptr->find_plugin<ReclamationResourceTaskPlugin>();
    m_reclamation_task_ptr->set_enable(true);
    if (plugin->get_enable()) {
        plugin->reset();
    }
    const bool result = InterfaceTask::run();
    // ProcessTask 的插件回调不传播返回值，必须将插件记录的失败传回整个任务链。
    return result && !plugin->failed();
}
