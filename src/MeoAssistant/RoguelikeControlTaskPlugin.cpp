#include "RoguelikeControlTaskPlugin.h"

bool asst::RoguelikeControlTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskExtraInfo
        || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }
    const std::string what = details.get("what", std::string());
    if (what != "ExceededLimit") {
        return false;
    }

    const std::string task = details.at("details").at("task").as_string();
    if (task == "Roguelike1Start" ||
        task == "Roguelike1StageTraderInvestConfirm" ||
        task == "Roguelike1StageTraderInvestSystemFull") {
        return true;
    }

    return false;
}

bool asst::RoguelikeControlTaskPlugin::_run()
{
    m_task_ptr->set_enable(false);
    return true;
}
