#include "CopilotListNotificationPlugin.h"

bool asst::CopilotListNotificationPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string task = details.at("details").at("task").as_string();
    if (task != "BattleStartAll" && task.starts_with("BattleStart")) {
        return true;
    }
    else {
        return false;
    }
}

bool asst::CopilotListNotificationPlugin::_run()
{
    json::value info = basic_info_with_what("CopilotListEnterSuccess");
    callback(AsstMsg::SubTaskExtraInfo, info);

    return true;
}
