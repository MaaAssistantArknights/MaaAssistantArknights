#include "RoguelikeDifficultySelectionTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

bool asst::RoguelikeDifficultySelectionTaskPlugin::load_params([[maybe_unused]] const json::value& params)
{
    // 深入调查和月度小队模式不需要选择难度
    if (m_config->get_mode() == RoguelikeMode::Exploration || m_config->get_mode() == RoguelikeMode::Squad) {
        return false;
    }

    auto opt = params.find<int>("difficulty");
    return opt && *opt != -1;
}

bool asst::RoguelikeDifficultySelectionTaskPlugin::verify(AsstMsg msg, const json::value& details) const
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
    if (task_view == "Roguelike@StartExplore") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeDifficultySelectionTaskPlugin::_run()
{
    LogTraceFunction;

    if (m_config->get_run_for_collectible()) {
        Log.info(__FUNCTION__, "| Running for collectible");
    }

    const int difficulty = m_config->get_run_for_collectible() ? 0 : m_config->get_difficulty();
    Log.info(__FUNCTION__, "| current_difficulty:", m_current_difficulty, "next difficulty:", difficulty);

    // 仅在插件记录的当前难度与目标难度不一致时重新选择难度
    if (m_current_difficulty != difficulty) {
        select_difficulty(difficulty);
    }

    return true;
}

int asst::RoguelikeDifficultySelectionTaskPlugin::detect_current_difficulty() const
{
    LogTraceFunction;

    const cv::Mat image = ctrler()->get_image();
    OCRer analyzer(image);
    analyzer.set_task_info("Roguelike@ChooseDifficulty_AnalyzeCurrentDifficulty");
    if (analyzer.analyze()) {
        const std::string text = analyzer.get_result().front().text;
        Log.info("Detected difficulty text:", text);
        int difficulty;
        if (!utils::chars_to_number(text, difficulty)) {
            Log.error("Failed to convert difficulty text to number. Text =", text);
            return 0;
        }
        return difficulty;
    }
    else {
        Log.error("OCR failed. Cannot detect difficulty.");
        return 0;
    }
}

bool asst::RoguelikeDifficultySelectionTaskPlugin::select_difficulty(const int difficulty)
{
    LogTraceFunction;

    if (difficulty == INT_MAX) {
        ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ChooseDifficultyEnter" }).run();
        ProcessTask(*this, { "SwipeToTheDown" }).run();
        ProcessTask(*this, { "SwipeToTheDown" }).run();
        m_current_difficulty = detect_current_difficulty();
    }
    else if (difficulty == 0) {
        ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ChooseDifficultyEnter" }).run();
        ProcessTask(*this, { "SwipeToTheUp" }).run();
        ProcessTask(*this, { "SwipeToTheUp" }).run();
        m_current_difficulty = detect_current_difficulty();
    }
    else {
        ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ChooseDifficultyEnter" }).run();
        m_current_difficulty = detect_current_difficulty();
        Log.info("Target difficulty:", difficulty);
        Log.info("Current difficulty:", m_current_difficulty);
        if (m_current_difficulty != difficulty) {
            if (m_current_difficulty < difficulty) {
                ProcessTask(*this, { "SwipeToTheDown" }).run();
                ProcessTask(*this, { "SwipeToTheDown" }).run();
            }
            std::vector<std::string> difficulty_list;
            for (int i = 20; i >= difficulty; --i) { // 难度识别内容为 20 ~ difficulty
                difficulty_list.push_back(std::to_string(i));
            }
            Task.get<OcrTaskInfo>(m_config->get_theme() + "@Roguelike@ChooseDifficulty_Specified")->text =
                difficulty_list;
            ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ChooseDifficulty_Specified", "Stop" }).run();
            m_current_difficulty = detect_current_difficulty();
        }
    }

    Log.info("Target difficulty:", difficulty);
    Log.info("Current difficulty:", m_current_difficulty);

    ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ChooseDifficultyConfirm" }).run();

    return true;
}
