#include "RoguelikeResetTaskPlugin.h"

#include "RuntimeStatus.h"

bool asst::RoguelikeResetTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    auto roguelike_name_opt = m_status->get_properties("roguelike_name");
    if (!roguelike_name_opt) {
        return false;
    }
    const auto& roguelike_name = roguelike_name_opt.value() + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@Start") {
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
