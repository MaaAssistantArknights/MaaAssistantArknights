#include "RoguelikeCiphertextBoardUseTaskPlugin.h"

#include "Config/Roguelike/RoguelikeCiphertextBoardConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

bool asst::RoguelikeCiphertextBoardUseTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (m_roguelike_theme.empty()) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    if (m_roguelike_theme != "Sami") {
        return false;
    }
    std::string mode = status()->get_properties(Status::RoguelikeMode).value();
    std::string task_name_pre = m_roguelike_theme + "@Roguelike@Stage";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;

    if (task_view.starts_with(task_name_pre)) {
        task_view.remove_prefix(task_name_pre.length());
    }
    // 不知道是不是不对纵向节点用比较好，先写上
    task_name_pre = "task_name_pre";
    if (task_view.starts_with("Vertical")) {
        task_view.remove_prefix(task_name_pre.length());
    }
    std::string task_name_suf = "AI6";
    if (task_view.ends_with(task_name_suf)) {
        task_view.remove_suffix(task_name_suf.length());
    }
    if (task_view == "CombatDps" || task_view == "EmergencyDps") {
        if (mode == "1" || mode == "4") {
            m_stage = "SkipBattle";
        }
        else if (mode == "0") {
            m_stage = "Battle";
        }
        return true;
    }
    if (task_view == "DreadfulFoe-5" && mode == "0") {
        m_stage = "Boss";
        return true;
    }
    if (task_view == "Trader" && mode == "0") {
        m_stage = "Trader";
        return true;
    }
    if (task_view == "Encounter" && mode == "0") {
        m_stage = "Encounter";
        return true;
    }
    if (task_view == "Gambling" && mode == "0") {
        m_stage = "Gambling";
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeCiphertextBoardUseTaskPlugin::_run()
{
    LogTraceFunction;

    std::string theme = m_roguelike_theme;

    std::vector<RoguelikeCiphertextBoardCombination> combination =
        RoguelikeCiphertextBoard.get_combination(m_roguelike_theme);

    std::string overview_str =
        status()->get_str(Status::RoguelikeCiphertextBoardOverview).value_or(json::value().to_string());
    json::value overview_json = json::parse(overview_str).value_or(json::value());
    auto& all_boards = overview_json.as_array();
    Log.debug(all_boards);
    for (const auto& usage : combination) {
        if (need_exit()) {
            return false;
        }
        if (m_stage == usage.usage) {
            // 用到没得用为止
            while (search_enable_pair(all_boards, usage)) {
                Log.debug("Use board pairs");
            }
            break;
        }
    }

    return true;
}

bool asst::RoguelikeCiphertextBoardUseTaskPlugin::search_enable_pair(const json::array& all_boards, const auto& usage)
{
    LogTraceFunction;

    // bool found_up = false;
    // bool found_down = false;

    for (auto& pair : usage.pairs) {
        // Log.debug(pair);
        // 遍历上板子
        for (auto& up_board : pair.up_board) {
            // 遍历下板子
            for (auto& down_board : pair.down_board) {
                // 两个板子同时存在all_boards中
                // Log.debug("上板子", up_board, "下板子", down_board);
                if (std::find(all_boards.begin(), all_boards.end(), up_board) != all_boards.end() &&
                    std::find(all_boards.begin(), all_boards.end(), down_board) != all_boards.end()) {
                    use_board(up_board, down_board);
                    return true;
                }
            }
        }
    }

    return false;
}

bool asst::RoguelikeCiphertextBoardUseTaskPlugin::use_board(const std::string& up_board, const std::string& down_board)
{
    LogTraceFunction;

    ProcessTask(*this, { m_roguelike_theme + "@Roguelike@CiphertextBoard" }).run();
    // todo:插入一个更新密文板overview
    search_and_click_board(up_board);
    search_and_click_board(down_board);
    // 点击节点位置,todo:根据坐标换算位置
    ProcessTask(*this, { m_roguelike_theme + "@Roguelike@CiphertextBoardUseOnStage" }).run();
    ProcessTask(*this, { m_roguelike_theme + "@Roguelike@CiphertextBoardUseConfirm" }).run();
    return true;
}

bool asst::RoguelikeCiphertextBoardUseTaskPlugin::search_and_click_board(const std::string& board)
{
    LogTraceFunction;

    while (!need_exit()) {
        OCRer analyzer(ctrler()->get_image());
        analyzer.set_task_info(m_roguelike_theme + "@Roguelike@CiphertextBoardUseOcr");
        analyzer.set_required({ board });
        if (!analyzer.analyze()) {
            // 往上滑
            slowly_swipe(true);
        }
        else {
            ctrler()->click(analyzer.get_result().front().rect);
            return true;
        }
    }
    return false;
}

void asst::RoguelikeCiphertextBoardUseTaskPlugin::slowly_swipe(bool to_up, int swipe_dist)
{
    std::string swipe_task_name =
        to_up ? "RoguelikeCiphertextBoardSlowlySwipeToTheUp" : "RoguelikeCiphertextBoardSlowlySwipeToTheDown";
    if (!ControlFeat::support(ctrler()->support_features(),
                              ControlFeat::PRECISE_SWIPE)) { // 不能精准滑动时不使用 swipe_dist 参数
        ProcessTask(*this, { swipe_task_name }).run();
        return;
    }

    if (!to_up) swipe_dist = -swipe_dist;
    auto swipe_task = Task.get(swipe_task_name);
    const Rect& StartPoint = swipe_task->specific_rect;
    ctrler()->swipe(StartPoint,
                    { StartPoint.x + swipe_dist - StartPoint.width, StartPoint.y, StartPoint.width, StartPoint.height },
                    swipe_task->special_params.empty() ? 0 : swipe_task->special_params.at(0),
                    (swipe_task->special_params.size() < 2) ? false : swipe_task->special_params.at(1),
                    (swipe_task->special_params.size() < 3) ? 1 : swipe_task->special_params.at(2),
                    (swipe_task->special_params.size() < 4) ? 1 : swipe_task->special_params.at(3));
    sleep(swipe_task->post_delay);
}
