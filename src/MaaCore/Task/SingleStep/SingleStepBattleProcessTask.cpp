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

    cv::Mat image = ctrler()->get_image();
    bool init = !check_in_battle(image);
    if (init) {
        clear();

        if (!calc_tiles_info(m_stage_name)) {
            Log.error("get stage info failed");
            return false;
        }
    }

    update_deployment(init, image);

    if (init) {
        to_group();
    }

    size_t action_size = m_actions.size();
    for (size_t i = 0; i < action_size && !need_exit() && m_in_battle; ++i) {
        const auto& action = m_actions.at(i);
        do_action(action, i);
    }

    return true;
}
