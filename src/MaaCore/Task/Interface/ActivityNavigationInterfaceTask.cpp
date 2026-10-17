#include "ActivityNavigationInterfaceTask.h"

#include "Config/Miscellaneous/TilePack.h"
#include "Task/Miscellaneous/ActivityNavigationTask.h"

asst::ActivityNavigationInterfaceTask::ActivityNavigationInterfaceTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_navigation(std::make_shared<ActivityNavigationTask>(callback, inst, TaskType))
{
    m_navigation->set_retry_times(0);
    m_subtasks.emplace_back(m_navigation);
}

bool asst::ActivityNavigationInterfaceTask::set_params(const json::value& params)
{
    const auto stage = params.find<std::string>("stage_id");
    if (!stage) {
        return false;
    }
    const auto map = Tile.find(*stage);
    return map && m_navigation->set_target(
                      *stage,
                      map->first.code,
                      params.get("is_raid", false),
                      params.get("client_type", std::string()));
}
