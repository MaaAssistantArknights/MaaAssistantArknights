#include "RoguelikeDifficultySelectionTaskPlugin.h"

#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeDifficultySelectionTaskPlugin::verify(AsstMsg msg, const json::value& details) const
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

    std::string theme = status()->get_properties(Status::RoguelikeTheme).value();
    std::string mode = status()->get_properties(Status::RoguelikeMode).value();
    // todo:以后可以根据传入的难度值选择难度?

    // 当前难度
    std::string recent_difficulty = status()->get_properties(Status::RoguelikeNeedChangeDifficulty).value();
    // 需要开局凹直升
    std::string start_with_elite_two = status()->get_properties(Status::RoguelikeStartWithEliteTwo).value();
    if (theme != "Phantom" && mode == "4") {
        if (recent_difficulty == "max") {
            // 到第三层退出，选最高难度开水壶
            ProcessTask(*this, { theme + "@Roguelike@ChooseDifficulty_Hardest" }).run();
            // 重置为最低难度，需要凹开局直升时，保证在难度max下寻找直升干员，延后到找不到精二重置
            if (start_with_elite_two == "false") {
                status()->set_properties(Status::RoguelikeNeedChangeDifficulty, "0");
            }
        }
        // 其他情况下开始，选最低难度
        else {
            ProcessTask(*this, { theme + "@Roguelike@ChooseDifficulty_Easiest" }).run();
            status()->set_properties(Status::RoguelikeNeedChangeDifficulty, "0");
        }
        ProcessTask(*this, { theme + "@Roguelike@ChooseDifficultyConfirm" }).run();
        ProcessTask(*this, { theme + "@Roguelike@StartExploreConfirm" }).run();
    }

    return true;
}
