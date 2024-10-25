#include "RoguelikeDifficultySelectionTaskPlugin.h"

#include "Config/TaskData.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

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

    auto mode = m_config->get_mode();
    // todo:以后可以根据传入的难度值选择难度?

    // 当前难度
    int difficulty = 5;
    // 是否不进行作战
    bool no_battle = m_config->get_only_start_with_elite_two() || m_config->get_first_floor_foldartal();
    if (m_config->get_theme() != RoguelikeTheme::Phantom && mode == RoguelikeMode::Collectible && !no_battle) {
        if (difficulty == INT_MAX) {
            ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ChooseDifficulty_Hardest" }).run();
        }
        else if (difficulty == 0) {
            ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ChooseDifficulty_Easiest" }).run();
        }
        else {
            ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ChooseDifficulty_Hardest" }).run();
            //  难度识别内容为 20 ~ difficulty
            std::vector<std::string> difficulty_list;
            for (int i = 20; i >= difficulty; --i) {
                difficulty_list.push_back(std::to_string(i));
            }
            Task.get<OcrTaskInfo>(m_config->get_theme() + "@Roguelike@ChooseDifficulty_specified")->text =
                difficulty_list;
            ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ChooseDifficulty_specified" }).run();
        }
        ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ChooseDifficultyConfirm" }).run();
    }

    return true;
}
