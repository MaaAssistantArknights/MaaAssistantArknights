#include "RoguelikeLevelTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Task/Roguelike/RoguelikeControlTaskPlugin.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

bool asst::RoguelikeLevelTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (!RoguelikeConfig::is_valid_theme(m_config->get_theme())) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = m_config->get_theme() + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@StartExplore" && !checked) {
        return true;
    }
    else if (task_view == "Roguelike@ClickToStartPoint" || task_view == "Roguelike@ClickToStartPointAfterFailed") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeLevelTaskPlugin::load_params(const json::value& params)
{
    m_stop_at_max = params.get("stop_at_max_level", false);
    return m_stop_at_max;
}

bool asst::RoguelikeLevelTaskPlugin::_run()
{
    LogTraceFunction;

    if (checked) { // 一轮肉鸽结束，下次StartExplore再查一次
        checked = false;
        return true;
    }

    if (m_stop_at_max) {
        if (ProcessTask(*this, { m_config->get_theme() + "@Roguelike@CheckLevel" }).run()) {
            if (ProcessTask(*this, { m_config->get_theme() + "@Roguelike@CheckLevelMax" }).run()) {
                stop_roguelike();
            }
            else {
                checked = true;
                ProcessTask(*this, { m_config->get_theme() + "@Roguelike@CheckLevelBack" }).run();
            }
        }
    }

    return true;
}

void asst::RoguelikeLevelTaskPlugin::stop_roguelike() const
{
    ProcessTask(*this, { m_config->get_theme() + "@Roguelike@CheckLevelBack" })
        .set_times_limit("Roguelike@StartExplore", 0)
        .run();
    m_task_ptr->set_enable(false);
}
