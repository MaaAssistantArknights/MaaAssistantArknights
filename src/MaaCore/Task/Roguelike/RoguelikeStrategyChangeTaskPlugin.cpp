#include "RoguelikeStrategyChangeTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

bool asst::RoguelikeStrategyChangeTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (m_roguelike_theme.empty()) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = m_roguelike_theme + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@LevelName") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeStrategyChangeTaskPlugin::_run()
{
    LogTraceFunction;

    std::string theme = m_roguelike_theme;
    std::string mode = status()->get_properties(Status::RoguelikeMode).value();

    if (mode == "0") {
        auto image = ctrler()->get_image();
        OCRer analyzer(image);
        analyzer.set_task_info(theme + "@Roguelike@LevelName_mode0");
        if (!analyzer.analyze()) {
            return false;
        }
        const auto& reached_level = analyzer.get_result().front().text;
        auto& levelname_list = Task.get<OcrTaskInfo>(theme + "@Roguelike@LevelName_mode0")->text;
        for (int i = 0; i < levelname_list.size(); i++) {
            if (levelname_list[i] == reached_level) {
                switch (i) {
                case 0:
                    // 第一层
                    Task.set_task_base(theme + "@Roguelike@Stages", theme + "@Roguelike@Stages_aggressive");
                    break;
                case 1:
                    // 中间层，发育
                    Task.set_task_base(theme + "@Roguelike@Stages", theme + "@Roguelike@Stages_pragmatic");
                    break;
                case 2:
                    // 高层，避战
                    Task.set_task_base(theme + "@Roguelike@Stages", theme + "@Roguelike@Stages_default");
                    break;
                default:
                    Task.set_task_base(theme + "@Roguelike@Stages", theme + "@Roguelike@Stages_default");
                    break;
                }
                break;
            }
        }
    }

    return true;
}
