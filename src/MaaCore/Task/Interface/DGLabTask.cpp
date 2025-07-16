#include "DGLabTask.h"
#include <Utils/Logger.hpp>

asst::DGLabTask::DGLabTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_battle_spy_ptr(std::make_shared<BattleSpyTask>(callback, inst, TaskType))
{
    LogTraceFunction;

    m_subtasks.emplace_back(m_battle_spy_ptr);
}

bool asst::DGLabTask::set_params(const json::value& params)
{
    params;

    // 一直监视，直到战斗结束
    m_battle_spy_ptr->set_wait_until_end(true);

    return true;
}
