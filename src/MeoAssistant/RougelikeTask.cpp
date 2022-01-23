#include "RougelikeTask.h"

#include "ProcessTask.h"
#include "RoguelikeFormationTask.h"
#include "BattleTask.h"

bool asst::RougelikeTask::_run()
{
    ProcessTask start_task(*this, { "Roguelike1Begin" });
    if (!start_task.run()) {
        return false;
    }

    if (need_exit()) {
        return false;
    }

    RoguelikeFormationTask formation_task(m_callback, m_callback_arg, m_task_chain);
    if (!formation_task.run()) {
        return false;
    }

    if (need_exit()) {
        return false;
    }

    BattleTask battle_task(m_callback, m_callback_arg, m_task_chain);
    if (!battle_task.run()) {
        return false;
    }

    return true;
}
