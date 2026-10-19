#include "ReplenishOriginiumShardTaskPlugin.h"

#include "Task/ProcessTask.h"

bool asst::ReplenishOriginiumShardTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskExtraInfo || details.get("subtask", std::string()) != "InfrastMfgTask") {
        return false;
    }

    // 换班完成后再补货：空设施点确认会提示 ｢至少配置一名干员｣，留下未执行的制造计划
    if (details.at("what").as_string() == "ShiftCompleted" &&
        details.at("details").at("product").as_string() == "OriginStone") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::ReplenishOriginiumShardTaskPlugin::_run()
{
    m_ran = true;
    ProcessTask task(*this, { "ReplenishToMax" });
    return task.run();
}
