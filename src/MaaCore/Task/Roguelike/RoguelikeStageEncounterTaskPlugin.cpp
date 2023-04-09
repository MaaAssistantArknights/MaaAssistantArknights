#include "RoguelikeStageEncounterTaskPlugin.h"

#include "Controller/Controller.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Utils/ImageIo.hpp"
#include "Config/TaskData.h"
#include "Config/Roguelike/RoguelikeStageEncounterConfig.h"
#include "Task/ProcessTask.h"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

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

    for (int j = 0; j < 2; ++j) {
        sleep(150);
        ctrler()->click(Point(500, 500)); // 先点一下让文字到左边
    }       
    std::string rogue_theme = status()->get_properties(Status::RoguelikeTheme).value();
    std::vector<RoguelikeEvent> events = RoguelikeStageEncounter.get_events(rogue_theme);
    const auto event_name_task_ptr = Task.get("Roguelike@StageEncounterOcr");
    event_name_task_ptr->roi.y = 460;
    sleep(event_name_task_ptr->pre_delay);

    constexpr int EventNameRetryTimes = 15;
    for (int i = 0; i != EventNameRetryTimes; ++i) {
        if (need_exit()) {
            return false;
        }
        cv::Mat image = ctrler()->get_image();
        cv::Mat grey_image, black_image;
        cv::cvtColor(image, grey_image, cv::COLOR_RGB2GRAY);
        cv::threshold(grey_image, black_image, 80, 255, 0); // 要做个二值化，不然识别效果很差
        cv::cvtColor(black_image, image, cv::COLOR_GRAY2RGB);

        OcrWithPreprocessImageAnalyzer name_analyzer(image);
        name_analyzer.set_task_info(event_name_task_ptr);
        event_name_task_ptr->roi.y += 5; // 因文本纵向位置不固定，这里用一个滑动窗进行ocr
        if (!name_analyzer.analyze()) {
            continue;
        }
        name_analyzer.sort_result_by_score();
        const std::string& text = name_analyzer.get_result().front().text;
        if (text.empty()) {
            continue;
        }

        for (const auto& event : events) {
            if (text.find(event.name) != std::string::npos) {
                Log.info("Event:", event.name, "choose option", event.default_choose);
                for (int j = 0; j < 2; ++j) {
                    ProcessTask(*this, { rogue_theme + "@Roguelike@OptionChoose" + 
                        std::to_string(event.option_num) + "-" +
                        std::to_string(event.default_choose) }).run();
                    sleep(300);
                }
                return true;
            }
        }
    }
    Log.info("Unknown Event");
    return true;
}
