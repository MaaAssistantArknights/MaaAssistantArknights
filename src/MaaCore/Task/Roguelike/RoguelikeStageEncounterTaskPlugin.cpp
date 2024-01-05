#include "RoguelikeStageEncounterTaskPlugin.h"

#include "Config/Roguelike/RoguelikeStageEncounterConfig.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"

bool asst::RoguelikeStageEncounterTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    // 安全屋，掷骰子之类的带选项的也都是视为不期而遇了
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

    auto mode = m_config->get_mode();
    std::vector<RoguelikeEvent> events = RoguelikeStageEncounter.get_events(m_config->get_theme());
    // 刷源石锭模式和烧水模式
    if (mode == RoguelikeMode::Investment || mode == RoguelikeMode::Collectible) {
        events = RoguelikeStageEncounter.get_events(m_config->get_theme() + "_deposit");
    }
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

    int special_val = 0;
    // 水月的不好识别，先试试萨米能不能用
    if (m_config->get_theme() == RoguelikeTheme::Sami) {
        OCRer analyzer(image);
        analyzer.set_task_info(m_config->get_theme() + "Roguelike@SpecialValRecognition");
        analyzer.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);
        analyzer.set_use_char_model(true);

        if (!analyzer.analyze()) {
            return false;
        }
        utils::chars_to_number(analyzer.get_result().front().text, special_val);
    }

    int choose_option = process_task(event, special_val);
    Log.info("Event:", event.name, "special_val", special_val, "choose option", choose_option);

    auto info = basic_info_with_what("RoguelikeEvent");
    info["details"]["name"] = event.name;
    info["details"]["default_choose"] = event.default_choose;
    info["details"]["special_val"] = special_val;
    info["details"]["choose"] = choose_option;
    callback(AsstMsg::SubTaskExtraInfo, info);
    for (int j = 0; j < 2; ++j) {
        ProcessTask(*this, { m_config->get_theme() + "@Roguelike@OptionChoose" + std::to_string(event.option_num) +
                             "-" + std::to_string(choose_option) })
            .run();
        sleep(300);
    }

    // 判断是否点击成功，成功进入对话后左上角的生命值会消失
    image = ctrler()->get_image();
    if (hp(image) == -1) {
        return true;
    }

    int max_time = event.option_num;
    while (max_time > 0) {
        for (int i = 0; i < max_time; ++i) {
            for (int j = 0; j < 2; ++j) {
                ProcessTask(*this, { m_config->get_theme() + "@Roguelike@OptionChoose" + std::to_string(max_time) +
                                     "-" + std::to_string(i) })
                    .run();
                sleep(300);
            }

            image = ctrler()->get_image();
            if (hp(image) == -1) {
                return true;
            }
        }
        // 没通关结局有些事件会少选项
        --max_time;
    }

    return false;
}
