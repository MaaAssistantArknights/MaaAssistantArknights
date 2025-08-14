#include "RoguelikeDifficultySelectionTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

bool asst::RoguelikeDifficultySelectionTaskPlugin::load_params([[maybe_unused]] const json::value& params)
{
    // 深入调查和月度小队模式不需要选择难度
    if (m_config->get_mode() == RoguelikeMode::Exploration || m_config->get_mode() == RoguelikeMode::Squad) {
        return false;
    }

    if (m_config->get_mode() == RoguelikeMode::Collectible) {
        if (m_config->get_difficulty() != -1) {
            m_collectible_difficulty = -1; // 当前难度只能用当前难度烧水
        }
        else {
            const std::string& squad = params.get("squad", "");
            const std::string& collectible_mode_squad = params.get("collectible_mode_squad", squad);
            if (m_config->get_theme() == RoguelikeTheme::JieGarden && collectible_mode_squad == "指挥分队" &&
                // 界园指挥分队可用 3 难快速烧水
                m_config->get_difficulty() >= 3) {
                m_collectible_difficulty = 3;
            }
            else {
                m_collectible_difficulty = 0;
            }
        }
    }

    /*auto opt = params.find<int>("difficulty");
    return opt && *opt != -1;*/
    return true;
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
    if (task_view.ends_with("Roguelike@GamePass")) {
        m_has_changed = false;
    }
    if (task_view == "Roguelike@StartExplore") { // 烧水时候调来调去的干脆不走
        return m_config->get_mode() == RoguelikeMode::Collectible || !m_has_changed;
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

    const int difficulty = m_config->get_run_for_collectible() ? m_collectible_difficulty : m_config->get_difficulty();
    Log.info(__FUNCTION__, "| current_difficulty:", m_current_difficulty, "next difficulty:", difficulty);

    // 仅在插件记录的当前难度与目标难度不一致时重新选择难度
    select_difficulty(difficulty);

    m_has_changed = true;
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
            return -1;
        }
        return difficulty;
    }
    else {
        Log.error("OCR failed. Cannot detect difficulty.");
        return -1;
    }
}

bool asst::RoguelikeDifficultySelectionTaskPlugin::select_difficulty(const int difficulty)
{
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();
    static std::unordered_set<std::string> initialized_themes;

    if (!initialized_themes.contains(theme)) {
        ProcessTask(*this, { theme + "@Roguelike@ChooseDifficultyEnter" }).run();

        // 第一次运行肉鸽或者重装游戏后没有难度按钮，先判断下有没有 Confirm，没有就点一下 StartExplore
        OCRer confirm_matcher(ctrler()->get_image());
        confirm_matcher.set_task_info(theme + "@Roguelike@ChooseDifficultyConfirm");
        if (!confirm_matcher.analyze()) {
            Log.warn("Failed to find difficulty selection UI. Try to click Roguelike@StartExplore.");
            Matcher start_explore_matcher(ctrler()->get_image());
            start_explore_matcher.set_task_info(theme + "@Roguelike@StartExplore");
            if (start_explore_matcher.analyze()) {
                ctrler()->click(start_explore_matcher.get_result().rect);
            }
            else {
                Log.error("Failed to find Roguelike@StartExplore button. Cannot proceed with difficulty selection.");
                return false;
            }
        }

        if (difficulty == m_current_difficulty) {
            Log.info("Current difficulty is already set to the target difficulty:", difficulty);
            ProcessTask(*this, { theme + "@Roguelike@ChooseDifficultyConfirm" }).run();
            initialized_themes.insert(theme);
            return true;
        }
        initialized_themes.insert(theme);
    }
    else {
        if (difficulty == m_current_difficulty) {
            Log.info("Current difficulty is already set to the target difficulty:", difficulty);
            return true;
        }
        ProcessTask(*this, { theme + "@Roguelike@ChooseDifficultyEnter" }).run();
    }

    if (difficulty == INT_MAX) {
        ProcessTask(*this, { "SwipeToTheDown" }).run();
        ProcessTask(*this, { "SwipeToTheDown" }).run();
        m_current_difficulty = detect_current_difficulty();
    }
    else if (difficulty == 0) {
        ProcessTask(*this, { "SwipeToTheUp" }).run();
        ProcessTask(*this, { "SwipeToTheUp" }).run();
        m_current_difficulty = detect_current_difficulty();
    }
    else {
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
            Task.get<OcrTaskInfo>(theme + "@Roguelike@ChooseDifficulty_Specified")->text = difficulty_list;
            ProcessTask(*this, { theme + "@Roguelike@ChooseDifficulty_Specified", "Stop" }).run();
            m_current_difficulty = detect_current_difficulty();
        }
    }

    Log.info("Target difficulty:", difficulty);
    Log.info("Current difficulty:", m_current_difficulty);

    ProcessTask(*this, { theme + "@Roguelike@ChooseDifficultyConfirm" }).run();

    return true;
}
