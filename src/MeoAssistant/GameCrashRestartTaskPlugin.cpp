#include "GameCrashRestartTaskPlugin.h"

#include "RuntimeStatus.h"

bool asst::GameCrashRestartTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart
        || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "RestartGameAndContinueFighting"
        && m_status->get_number("LastStartButton2")) {
        return true;
    }
    else {
        return false;
    }
}
