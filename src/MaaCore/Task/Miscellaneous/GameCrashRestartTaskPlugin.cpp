#include "GameCrashRestartTaskPlugin.h"

#include "Status.h"

bool asst::GameCrashRestartTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    // 没啥用，还经常误报
    // if (details.at("details").at("task").as_string() == "Fight@RestartGameAndContinue") {
    //     return true;
    // }
    // else {
    return false;
    //}
}
