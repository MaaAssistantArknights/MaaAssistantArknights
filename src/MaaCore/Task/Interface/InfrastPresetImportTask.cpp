#include "InfrastPresetImportTask.h"
#include "Task/Infrast/InfrastPresetWriteTask.h"

asst::InfrastPresetImportTask::InfrastPresetImportTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_writer(std::make_shared<InfrastPresetWriteTask>(callback, inst, TaskType))
{
    m_writer->set_retry_times(0);
    m_subtasks.emplace_back(m_writer);
}

bool asst::InfrastPresetImportTask::set_params(const json::value& params)
{
    return !m_running && m_writer->configure(params);
}
