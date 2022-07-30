#include "RoguelikeResetTaskPlugin.h"

#include "RuntimeStatus.h"

bool asst::RoguelikeResetTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart
    || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "Roguelike1Start") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeResetTaskPlugin::_run()
{
    // 简单粗暴，后面如果多任务间有联动可能要改改
    m_status->clear_number();
    m_status->clear_str();
    return true;
}
