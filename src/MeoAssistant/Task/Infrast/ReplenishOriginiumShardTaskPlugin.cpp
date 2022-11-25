#include "ReplenishOriginiumShardTaskPlugin.h"

#include "Task/ProcessTask.h"

bool asst::ReplenishOriginiumShardTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskExtraInfo || details.get("subtask", std::string()) != "InfrastMfgTask") {
        return false;
    }

    if (details.at("what").as_string() == "ProductOfFacility" &&
        details.at("details").at("product").as_string() == "OriginStone") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::ReplenishOriginiumShardTaskPlugin::_run()
{
    ProcessTask task(*this, { "ReplenishToMax" });
    return task.run();
}
