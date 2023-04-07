#include "SingleStepBattleProcessTask.h"

#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::SingleStepBattleProcessTask::set_stage_name_cache(const std::string& stage_name)
{
    LogTraceFunction;

    if (!Tile.find(stage_name)) {
        Log.error("get stage info failed", stage_name);
        return false;
    }
    m_stage_name_cache = stage_name;
    return true;
}

void asst::SingleStepBattleProcessTask::set_actions(Actions actions)
{
    m_actions = std::move(actions);
}

bool asst::SingleStepBattleProcessTask::_run()
{
    LogTraceFunction;

    if (!calc_tiles_info(m_stage_name_cache)) {
        Log.error("get stage info failed", m_stage_name_cache);
        return false;
    }

    size_t action_size = m_actions.size();
    for (size_t i = 0; i < action_size && !need_exit(); ++i) {
        const auto& action = m_actions.at(i);
        do_action(action, i);
    }

    return true;
}
