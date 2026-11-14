#include "SeasonalTask.h"

#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

asst::SeasonalTask::SeasonalTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType)
{
    LogTraceFunction;
}

bool asst::SeasonalTask::set_params(const json::value& params)
{
    LogTraceFunction;

    m_spa_rounds = params.get("spa_rounds", 0);
    if (m_spa_rounds < 0) {
        LogWarn << __FUNCTION__ << "| invalid spa_rounds:" << m_spa_rounds << ", reset to 0";
        m_spa_rounds = 0;
    }
    LogInfo << __FUNCTION__ << "| spa_rounds:" << m_spa_rounds;
    return true;
}

bool asst::SeasonalTask::run()
{
    LogTraceFunction;

    if (m_spa_rounds > 0) {
        // 卫戍协议：极速模拟，每完成一局（返回主界面）记一轮，打满后整条链自然结束
        ProcessTask spa_task(*this, { "MiniGame@SPA@Begin" });
        spa_task.set_times_limit("MiniGame@SPA@ReturnHome", m_spa_rounds, ProcessTask::TimesLimitType::Post);
        if (!spa_task.run()) {
            return false;
        }
    }

    // TODO: 争锋频道（MiniGame@ALL@GreenGrass@DuelChannel@Begin），终点为打满轮数或完成每日奖励
    return true;
}
