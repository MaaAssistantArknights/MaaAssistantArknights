#include "DepotTask.h"

#include "Task/Miscellaneous/DepotRecognitionTask.h"
#include "Task/ProcessTask.h"

#include "Config/Miscellaneous/ItemConfig.h"

#include "Utils/Logger.hpp"

asst::DepotTask::DepotTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      m_recognition_task(std::make_shared<DepotRecognitionTask>(m_callback, m_inst, TaskType))
{
    LogTraceFunction;

    m_process_task = std::make_shared<ProcessTask>(m_callback, m_inst, TaskType);
    m_process_task->set_retry_times(2);
    m_subtasks.emplace_back(m_process_task);

    m_recognition_task->set_retry_times(0);
    m_subtasks.emplace_back(m_recognition_task);
}

bool asst::DepotTask::set_params(const json::value& params) {
    LogTraceFunction;
    
    const auto mode = params.get("mode", "Material");
    if (mode == "Material") {
        m_recognition_task->set_ordered_item_id(
            ItemData.get_ordered_item_id_filtered_by_classify_type({ ItemClassifyType::Material }));
        m_process_task->set_tasks({ "DepotMaterialBegin" }).set_ignore_error(true);
    }
    else if (mode == "All") {
        m_recognition_task->set_ordered_item_id(ItemData.get_ordered_item_id_filtered_by_classify_type(
            { ItemClassifyType::Material, ItemClassifyType::Normal }));
        m_process_task->set_tasks({ "DepotAllBegin" }).set_ignore_error(true);
    }

    return true;
}
