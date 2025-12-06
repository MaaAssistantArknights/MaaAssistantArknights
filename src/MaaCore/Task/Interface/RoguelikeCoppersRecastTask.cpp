#include "RoguelikeCoppersRecastTask.h"

#include "Task/Roguelike/JieGarden/RoguelikeCoppersRecastPlugin.h"
#include "Utils/Logger.hpp"

asst::RoguelikeCoppersRecastTask::RoguelikeCoppersRecastTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_recast_plugin_ptr(std::make_shared<RoguelikeCoppersRecastPlugin>(m_callback, m_inst, TaskType))
{
    LogTraceFunction;

    m_subtasks.emplace_back(m_recast_plugin_ptr);
}

bool asst::RoguelikeCoppersRecastTask::set_params(const json::value& params)
{
    LogTraceFunction;

    if (!m_recast_plugin_ptr) {
        return false;
    }

    m_recast_plugin_ptr->clear_conditions();
    return m_recast_plugin_ptr->configure(params);
}
