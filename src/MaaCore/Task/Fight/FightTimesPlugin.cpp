#include "FightTimesPlugin.h"

#include "Task/ProcessTask.h"

bool asst::FightTimesPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    return !inited && details.at("details").at("task").as_string().ends_with("StartButton1");
}

bool asst::FightTimesPlugin::_run()
{
    // 目前也可以做成subtask，但是给以后糊屎留个地方
    bool result = ProcessTask(*this, { "FightSeries-Default-1", "FightSeries-Icon", "Stop" }).run();

    if (!result) {
        return false;
    }

    inited = true;
    return true;
}
