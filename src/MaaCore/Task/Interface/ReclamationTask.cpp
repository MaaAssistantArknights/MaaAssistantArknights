#include "ReclamationTask.h"

#include "Common/AsstBattleDef.h"
#include "Config/TaskData.h"
#include "Task/ProcessTask.h"

// 通用配置及插件
#include "Task/Reclamation/ReclamationConfig.h"

// 尚未启用的配置及插件
#include "Task/Reclamation/ReclamationBattlePlugin.h"
#include "Task/Reclamation/ReclamationConclusionReportPlugin.h"
#include "Task/Reclamation/ReclamationControlTask.h"

#include "Utils/Logger.hpp"

asst::ReclamationTask::ReclamationTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_reclamation_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType)),
    m_config_ptr(std::make_shared<ReclamationConfig>())
{
    LogTraceFunction;

    m_subtasks.emplace_back(m_reclamation_task_ptr);
}

bool asst::ReclamationTask::set_params(const json::value& params)
{
    LogTraceFunction;

    if (!m_config_ptr->verify_and_load_params(params)) {
        return false;
    }

    const std::string& theme = m_config_ptr->get_theme();
    const ReclamationMode& mode = m_config_ptr->get_mode();

    if (theme == ReclamationTheme::Fire) {
        Log.info(__FUNCTION__, "Reclamation Algorithm theme", theme, "is no longer available");
        m_reclamation_task_ptr->set_tasks({ "Stop" });
        return true;
    }

    switch (mode) {
    case ReclamationMode::ProsperityNoSave:
        m_reclamation_task_ptr->set_tasks({ theme + "@RA@ProsperityNoSave" });
        break;
    case ReclamationMode::ProsperityInSave:
        m_reclamation_task_ptr->set_tasks({ theme + "@RA@ProsperityInSave" });
        const std::string& tool_to_craft = m_config_ptr->get_tool_to_craft();
        Task.get<OcrTaskInfo>(theme + "@Reclamation@ProsperityInSave@ClickTool")->text = { tool_to_craft };
        break;
    }

    return true;
}
