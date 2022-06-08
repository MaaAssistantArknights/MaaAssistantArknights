#include "GameCrashRestartTaskPlugin.h"

bool asst::GameCrashRestartTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted
        || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "MaybeCrashAndRestartGame") {
        return true;
    }
    else {
        return false;
    }
}
