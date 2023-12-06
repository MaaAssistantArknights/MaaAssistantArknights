#include "RoguelikeFoldartalUseTaskPlugin.h"

#include "Config/Roguelike/RoguelikeFoldartalConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

bool asst::RoguelikeFoldartalUseTaskPlugin::verify(const AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (!RoguelikeConfig::is_valid_theme(m_config->get_theme())) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    if (m_config->get_theme() != RoguelikeTheme::Sami) {
        return false;
    }
    auto mode = m_config->get_mode();
    std::string task_name_pre = m_config->get_theme() + "@Roguelike@Stage";
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
    const std::string task_name_suf = "AI6";
    if (task_view.ends_with(task_name_suf)) {
        task_view.remove_suffix(task_name_suf.length());
    }
    if (task_view == "CombatDps" || task_view == "EmergencyDps" || task_view == "FerociousPresage") {
        if (mode == RoguelikeMode::Investment || mode == RoguelikeMode::Collectible) {
            m_stage = "SkipBattle";
        }
        else if (mode == RoguelikeMode::Exp) {
            m_stage = "Battle";
        }
        return true;
    }
    if (task_view == "DreadfulFoe-5" && mode == RoguelikeMode::Exp) {
        m_stage = "Boss";
        return true;
    }
    if (task_view == "Trader" && mode == RoguelikeMode::Exp) {
        m_stage = "Trader";
        return true;
    }
    if (task_view == "Encounter" && mode == RoguelikeMode::Exp) {
        m_stage = "Encounter";
        return true;
    }
    if ((task_view == "Gambling" || task_view == "EmergencyTransportation" || task_view == "WindAndRain") &&
        mode == RoguelikeMode::Exp) {
        m_stage = "Gambling";
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeFoldartalUseTaskPlugin::_run()
{
    LogTraceFunction;

    std::vector<RoguelikeFoldartalCombination> combination = RoguelikeFoldartal.get_combination(m_config->get_theme());

    auto foldartal_list = m_config->get_foldartal();
    Log.debug("All foldartal got yet:", foldartal_list);
    if (const auto it = ranges::find_if(combination,
                                        [&](const RoguelikeFoldartalCombination& usage) { return m_stage == usage.usage; });
        it != combination.end()) {
        search_enable_pair(foldartal_list, *it);
    }

    return true;
}

bool asst::RoguelikeFoldartalUseTaskPlugin::search_enable_pair(std::vector<std::string>& list,
                                                               const asst::RoguelikeFoldartalCombination& usage)
{
    LogTraceFunction;
    auto check_pair = [&](const auto& pair) {
        // 存储需要跳过的板子
        std::vector<std::string> boards_to_remove;
        // 遍历上板子
        for (const std::string& up_board : pair.up_board) {
            if (need_exit()) {
                break;
            }
            auto iter_up = ranges::find(list, up_board);
            if (iter_up == list.end() || ranges::find(boards_to_remove, up_board) != boards_to_remove.end()) {
                continue;
            }

            // 遍历下板子
            for (const std::string& down_board : pair.down_board) {
                if (need_exit()) {
                    break;
                }
                auto iter_down = ranges::find(list, down_board);
                if (iter_down == list.end() || ranges::find(boards_to_remove, down_board) != boards_to_remove.end()) {
                    continue;
                }
                const auto result = use_board(up_board, down_board);
                // 直接结束任务
                if (result == use_board_result::click_foldartal_error) {
                    Log.error("Click foldartal error!");
                    return;
                }
                if (result == use_board_result::unknown_error) {
                    Log.error("Unknown error!");
                    return;
                }
                // 涉及上板子的错误，跳出循环
                // 通常是预见板子用不了
                if (result == use_board_result::stage_not_found) {
                    boards_to_remove.push_back(up_board);
                    boards_to_remove.push_back(down_board);
                    Log.error("Stage not found!");
                    break;
                }
                if (result == use_board_result::up_board_not_found) {
                    list.erase(iter_up);
                    Log.error("Up board not found!Delete up board:", up_board);
                    break;
                }
                // 涉及下板子的错误，继续循环
                if (result == use_board_result::down_board_not_found) {
                    list.erase(iter_down);
                    Log.error("Down board not found!Delete down board:", down_board);
                    continue;
                }
                // 正常使用板子，用完删除上板子和下板子
                if (result == use_board_result::use_board_result_success) {
                    list.erase(iter_up);
                    list.erase(iter_down);
                    Log.debug("Board pair used, up:", up_board, ", down:", down_board);
                }
            }
        }
    };

    ranges::for_each(usage.pairs, check_pair);

    return false;
}

asst::RoguelikeFoldartalUseTaskPlugin::use_board_result asst::RoguelikeFoldartalUseTaskPlugin::use_board(
    const std::string& up_board, const std::string& down_board) const
{
    Log.trace("Try to use the board pair", up_board, down_board);

    if (!ProcessTask(*this, { m_config->get_theme() + "@Roguelike@Foldartal" }).run()) {
        return use_board_result::click_foldartal_error;
    }

    swipe_to_top();
    // todo:插入一个滑动时顺便更新密文板overview,因为有的板子可以用两次
    if (!search_and_click_board(up_board)) {
        return use_board_result::up_board_not_found;
    }
    if (!search_and_click_board(down_board)) {
        return use_board_result::down_board_not_found;
    }
    if (!search_and_click_stage()) {
        return use_board_result::stage_not_found;
    }

    if (ProcessTask(*this, { m_config->get_theme() + "@Roguelike@FoldartalUseConfirm" }).run()) {
        return use_board_result::use_board_result_success;
    }

    ProcessTask(*this, { m_config->get_theme() + "@Roguelike@FoldartalBack" }).run();
    return use_board_result::unknown_error;
}

bool asst::RoguelikeFoldartalUseTaskPlugin::search_and_click_board(const std::string& board) const
{
    Log.trace("Search and click the board", board);

    constexpr int max_retry = 10;
    int try_time = 0;
    while (try_time < max_retry && !need_exit()) {
        OCRer analyzer(ctrler()->get_image());
        std::string task_name = m_config->get_theme() + "@Roguelike@FoldartalUseOcr";
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
    return false;
}

bool asst::RoguelikeFoldartalUseTaskPlugin::search_and_click_stage() const
{
    Log.trace("Try to click stage", m_stage);
    // todo:根据坐标换算位置,根据节点类型设置识别优先度

    // 重置到最左边
    ProcessTask(*this, { "SwipeToTheLeft" }).run();
    sleep(1000);

    // 节点会闪烁，所以这里不用单次Match
    if (ProcessTask(*this, { m_config->get_theme() + "@Roguelike@FoldartalUseOnStage" }).run()) {
        return true;
    }
    // 滑到最右边，萨米的地图只有两页，暂时先这么糊着，出现识别不到再写循环
    ProcessTask(*this, { "SwipeToTheRight" }).run();
    sleep(1000);
    if (ProcessTask(*this, { m_config->get_theme() + "@Roguelike@FoldartalUseOnStage" }).run()) {
        return true;
    }
    return false;
}

void asst::RoguelikeFoldartalUseTaskPlugin::swipe_to_top() const
{
    LogTraceFunction;

    int max_retry = 10;
    int try_time = 0;
    // 找到"布局"就到了最上面
    while (try_time < max_retry && !need_exit()) {
        while (try_time < max_retry && !need_exit()) {
            OCRer analyzer(ctrler()->get_image());
            analyzer.set_task_info(m_config->get_theme() + "@Roguelike@FoldartalUseOcr");
            if (analyzer.analyze()) {
                return;
            }
            else {
                // 往下滑
                ProcessTask(*this, { "RoguelikeFoldartalSwipeToTheDown" }).run();
            }
            try_time++;
        }
    }
}

void asst::RoguelikeFoldartalUseTaskPlugin::slowly_swipe(const bool direction, int swipe_dist) const
{
    std::string swipe_task_name = direction ? "RoguelikeFoldartalSlowlySwipeToTheUp" : "RoguelikeFoldartalSwipeToTheDown";
    if (!ControlFeat::support(ctrler()->support_features(),
                              ControlFeat::PRECISE_SWIPE)) { // 不能精准滑动时不使用 swipe_dist 参数
        ProcessTask(*this, { swipe_task_name }).run();
        return;
    }

    if (!direction) swipe_dist = -swipe_dist;
    const auto swipe_task = Task.get(swipe_task_name);
    const Rect& start_point = swipe_task->specific_rect;
    ctrler()->swipe(start_point,
                    { start_point.x + swipe_dist - start_point.width, start_point.y, start_point.width, start_point.height },
                    swipe_task->special_params.empty() ? 0 : swipe_task->special_params.at(0),
                    (swipe_task->special_params.size() < 2) ? false : swipe_task->special_params.at(1),
                    (swipe_task->special_params.size() < 3) ? 1 : swipe_task->special_params.at(2),
                    (swipe_task->special_params.size() < 4) ? 1 : swipe_task->special_params.at(3));
    sleep(swipe_task->post_delay);
}
