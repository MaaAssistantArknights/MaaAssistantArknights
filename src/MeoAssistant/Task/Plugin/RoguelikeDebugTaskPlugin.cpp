#include "RoguelikeDebugTaskPlugin.h"

#include "Controller.h"

bool asst::RoguelikeDebugTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (msg == AsstMsg::SubTaskError) {
        return true;
    }

    std::string task = details.get("details", "task", std::string());
    if (msg == AsstMsg::SubTaskStart && details.get("subtask", std::string()) == "ProcessTask" &&
        task == "Roguelike1ExitThenAbandon") {
        return true;
    }

    return false;
}

bool asst::RoguelikeDebugTaskPlugin::_run()
{
    return save_img("debug/roguelike/");
}
