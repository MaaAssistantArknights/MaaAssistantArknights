#include "SingleStepBattleProcessTask.h"

#include "Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

void asst::SingleStepBattleProcessTask::set_actions(Actions actions)
{
    m_actions = std::move(actions);
}

bool asst::SingleStepBattleProcessTask::_run()
{
    LogTraceFunction;

    if (!calc_tiles_info(m_stage_name)) {
        Log.error("get stage info failed");
        return false;
    }

    size_t action_size = m_actions.size();
    for (size_t i = 0; i < action_size && !need_exit(); ++i) {
        const auto& action = m_actions.at(i);
        do_action(action, i);
    }

    return true;
}
