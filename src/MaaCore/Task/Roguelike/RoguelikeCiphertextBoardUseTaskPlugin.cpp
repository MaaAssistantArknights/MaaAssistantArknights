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
    if (task_view == "CombatDps" || task_view == "EmergencyDps" || task_view == "FerociousPresage") {
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
    if ((task_view == "Gambling" || task_view == "EmergencyTransportation" || task_view == "WindAndRain") &&
        mode == "0") {
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
    m_all_boards = overview_json.as_array();
    Log.debug(m_all_boards);
    for (const auto& usage : combination) {
        if (need_exit()) {
            return false;
        }
        if (m_stage == usage.usage) {
            // 用到没得用为止,但是只要没有使用成功就不继续使用
            while (search_enable_pair(usage) && !m_board_use_error && !need_exit()) {
                Log.info("Use board pairs");
            }
            m_board_use_error = false;
            break;
        }
    }

    return true;
}

bool asst::RoguelikeCiphertextBoardUseTaskPlugin::search_enable_pair(const auto& usage)
{
    LogTraceFunction;

    for (const auto& pair : usage.pairs) {
        // 遍历上板子
        for (const auto& up_board : pair.up_board) {
            // 遍历下板子
            for (const auto& down_board : pair.down_board) {
                if (board_pair(up_board, down_board)) return true;
            }
        }
    }

    return false;
}

bool asst::RoguelikeCiphertextBoardUseTaskPlugin::board_pair(const std::string& up_board, const std::string& down_board)
{

    auto iter_up = std::find(m_all_boards.begin(), m_all_boards.end(), up_board);
    auto iter_down = std::find(m_all_boards.begin(), m_all_boards.end(), down_board);
    // 如果两个板子同时存在m_all_boards中
    if (iter_up != m_all_boards.end() && iter_down != m_all_boards.end()) {
        if (use_board(up_board, down_board)) {
            // 用完删除上板子和下板子
            m_all_boards.erase(iter_up);
            iter_down = std::find(m_all_boards.begin(), m_all_boards.end(), down_board);
            m_all_boards.erase(iter_down);
            status()->set_str(Status::RoguelikeCiphertextBoardOverview, m_all_boards.to_string());
            Log.debug("Board pairs used");
        }
        return true;
    }
    return false;
}

bool asst::RoguelikeCiphertextBoardUseTaskPlugin::use_board(const std::string& up_board, const std::string& down_board)
{
    Log.trace("Try to use the board pair", up_board, down_board);

    if (ProcessTask(*this, { m_roguelike_theme + "@Roguelike@CiphertextBoard" }).run()) {
        swipe_to_top();
        // todo:插入一个滑动时顺便更新密文板overview,因为有的板子可以用两次
        if (search_and_click_board(up_board) && search_and_click_board(down_board)) {
            search_and_click_stage();
            if (ProcessTask(*this, { m_roguelike_theme + "@Roguelike@CiphertextBoardUseConfirm" }).run()) {
                return true;
            }
        }
        ProcessTask(*this, { m_roguelike_theme + "@Roguelike@CiphertextBoardBack" }).run();
    }
    m_board_use_error = true;
    return false;
}

bool asst::RoguelikeCiphertextBoardUseTaskPlugin::search_and_click_board(const std::string& board)
{
    Log.trace("Search and click the board", board);

    int max_retry = 10;
    int try_time = 0;
    while (try_time < max_retry && !need_exit()) {
        OCRer analyzer(ctrler()->get_image());
        std::string task_name = m_roguelike_theme + "@Roguelike@CiphertextBoardUseOcr";
        analyzer.set_task_info(task_name);
        analyzer.set_required({ board });
        if (!analyzer.analyze()) {
            // 往上滑
            slowly_swipe(true);
        }
        else {
            ctrler()->click(analyzer.get_result().front().rect.move(Task.get(task_name)->rect_move));
            return true;
        }
        try_time++;
    }
    m_board_use_error = true;
    return false;
}

bool asst::RoguelikeCiphertextBoardUseTaskPlugin::search_and_click_stage()
{
    Log.trace("Try to click stage", m_stage);
    // todo:根据坐标换算位置,根据节点类型设置识别优先度

    // 重置到最左边
    ProcessTask(*this, { "SwipeToTheRight" }).run();
    // 节点会闪烁，所以这里不用单次Match
    if (ProcessTask(*this, { m_roguelike_theme + "@Roguelike@CiphertextBoardUseOnStage" }).run()) {
        return true;
    }
    // 滑到最右边，萨米的地图只有两页，暂时先这么糊着，出现识别不到再写循环
    ProcessTask(*this, { "SwipeToTheLeft" }).run();
    if (ProcessTask(*this, { m_roguelike_theme + "@Roguelike@CiphertextBoardUseOnStage" }).run()) {
        return true;
    }
    return false;
}

void asst::RoguelikeCiphertextBoardUseTaskPlugin::swipe_to_top()
{
    LogTraceFunction;

    int max_retry = 10;
    int try_time = 0;
    // 找到布局"就到了最上面
    while (try_time < max_retry && !need_exit()) {
    while (try_time < max_retry && !need_exit()) {
        OCRer analyzer(ctrler()->get_image());
        analyzer.set_task_info(m_roguelike_theme + "@Roguelike@CiphertextBoardUseOcr");
        if (analyzer.analyze()) {
            return;
        }
        else {
            // 往下滑
            ProcessTask(*this, { "RoguelikeCiphertextBoardSwipeToTheDown" }).run();
        }
        try_time++;
    }
    m_board_use_error = true;
}

void asst::RoguelikeCiphertextBoardUseTaskPlugin::slowly_swipe(bool to_up, int swipe_dist)
{
    std::string swipe_task_name =
        to_up ? "RoguelikeCiphertextBoardSlowlySwipeToTheUp" : "RoguelikeCiphertextBoardSwipeToTheDown";
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
