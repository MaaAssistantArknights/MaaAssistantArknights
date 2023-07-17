#include "RoguelikeStageEncounterTaskPlugin.h"

#include "Config/Roguelike/RoguelikeStageEncounterConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

bool asst::RoguelikeStageEncounterTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    auto roguelike_name_opt = status()->get_properties(Status::RoguelikeTheme);
    if (!roguelike_name_opt) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = std::move(roguelike_name_opt.value()) + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@StageEncounterJudgeOption") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeStageEncounterTaskPlugin::_run()
{
    LogTraceFunction;

    std::string rogue_theme = status()->get_properties(Status::RoguelikeTheme).value();
    std::vector<RoguelikeEvent> events = RoguelikeStageEncounter.get_events(rogue_theme);
    std::vector<std::string> event_names;
    std::unordered_map<std::string, RoguelikeEvent> event_map;
    for (const auto& event : events) {
        event_names.emplace_back(event.name);
        event_map.emplace(event.name, event);
    }
    const auto event_name_task_ptr = Task.get("Roguelike@StageEncounterOcr");
    sleep(event_name_task_ptr->pre_delay);

    if (need_exit()) {
        return false;
    }
    cv::Mat image = ctrler()->get_image();
    OCRer name_analyzer(image);
    name_analyzer.set_task_info(event_name_task_ptr);
    name_analyzer.set_required(event_names);
    if (!name_analyzer.analyze()) {
        Log.info("Unknown Event");
        return true;
    }
    const auto& resultVec = name_analyzer.get_result();
    if (resultVec.empty()) {
        Log.info("Unknown Event");
        return true;
    }
    std::string text = resultVec.front().text;

    RoguelikeEvent event = event_map.at(text);
    Log.info("Event:", event.name, "choose option", event.default_choose);
    for (int j = 0; j < 2; ++j) {
        ProcessTask(*this, { rogue_theme + "@Roguelike@OptionChoose" + 
            std::to_string(event.option_num) + "-" + std::to_string(event.default_choose) }).run();
        sleep(300);
    }

    return true;
}
