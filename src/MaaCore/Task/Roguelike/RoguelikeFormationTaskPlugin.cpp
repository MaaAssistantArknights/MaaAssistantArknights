#include "RoguelikeFormationTaskPlugin.h"

#include "Utils/Ranges.hpp"

#include "Config/Roguelike/RoguelikeRecruitConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeFormationTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (m_roguelike_theme.empty()) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = m_roguelike_theme + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@QuickFormation") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeFormationTaskPlugin::_run()
{
    RoguelikeFormationImageAnalyzer formation_analyzer(ctrler()->get_image());
    if (!formation_analyzer.analyze()) {
        return false;
    }

    size_t pre_selected = 0;
    size_t select_count = 0;
    for (const auto& oper : formation_analyzer.get_result()) {
        if (oper.selected) {
            ++pre_selected;
            continue;
        }
        ctrler()->click(oper.rect);
        ++select_count;
    }
    Log.info(__FUNCTION__, "pre_selected: ", pre_selected, " select: ", select_count);

    // 以下情况清空重选（游戏会自动排序）
    // 1. 这一页选满 8 个了
    // 2. select 有值，但是重新识别后发现总的选择数量并没有增加（说明上面的click都没生效）
    bool reselect = false;

    if (pre_selected == MaxNumOfOperPerPage) {
        reselect = true;
    }
    if (!reselect && select_count != 0) {
        sleep(Task.get("RoguelikeQuickFormationDelay")->post_delay);
        formation_analyzer.set_image(ctrler()->get_image());

        if (!formation_analyzer.analyze()) {
            Log.warn("RoguelikeFormationImageAnalyzer re analyze failed");
            return true;
        }

        auto& new_result = formation_analyzer.get_result();
        size_t new_selected_count = ranges::count_if(new_result, [](const auto& oper) { return oper.selected; });
        // 说明 select_count 计数没生效，即都没点上
        if (new_selected_count == pre_selected) {
            reselect = true;
        }
    }

    if (reselect) {
        clear_and_reselect();
    }

    return true;
}

void asst::RoguelikeFormationTaskPlugin::clear_and_reselect()
{
    // 清空并退出游戏会自动按等级重新排序
    ProcessTask(*this, { "RoguelikeQuickFormationClearAndReselect" }).run();

    oper_list.clear();
    cur_page = 1;

    while (analyze()) { // 返回true说明新增了干员，可能还有下一页
        ProcessTask(*this, { "RoguelikeRecruitOperListSlowlySwipeToTheRight" }).run();
        cur_page++;
    }

    cur_page--; // 最后多划了一下，退回最后一页的页码
    max_page = cur_page;
    Log.info(__FUNCTION__, "max_page: ", max_page, " oper_count: ", oper_list.size());

    std::vector<asst::RoguelikeFormationImageAnalyzer::FormationOper> sorted_oper_list;
    const auto& team_complete_condition = RoguelikeRecruit.get_team_complete_info(m_roguelike_theme);
    const auto& group_list = RoguelikeRecruit.get_group_info(m_roguelike_theme);
    for (const auto& condition : team_complete_condition) { // 优先选择阵容核心干员
        int count = 0;
        int require = condition.threshold;
        for (const std::string& group_name : condition.groups) {
            for (const auto& oper : oper_list) {
                std::vector<int> group_ids = RoguelikeRecruit.get_group_id(m_roguelike_theme, oper.name);
                for (const auto& group_id : group_ids) {
                    std::string oper_group = group_list[group_id];
                    if (oper_group == group_name) {
                        sorted_oper_list.emplace_back(oper);
                        count++;
                        break; // 每个干员只能选一次
                    }
                }
                if (count == require) break;
            }
            if (count == require) break;
        }
    }
    auto select_others = [&](bool process_reserve_oper) { // 然后按默认排序选择
        for (const auto& oper : oper_list) {
            if (sorted_oper_list.size() >= 13) break;
            if (process_reserve_oper != oper.name.starts_with("预备干员")) continue;
            bool already_exist = false;
            for (const auto& existing_oper : sorted_oper_list) {
                if (oper.name == existing_oper.name) already_exist = true;
            }
            if (!already_exist) {
                sorted_oper_list.emplace_back(oper);
            }
        }
    };
    select_others(false); // 非预备干员
    select_others(true);  // 预备干员

    for (const auto& oper : sorted_oper_list) {
        select(oper);
    }
}

bool asst::RoguelikeFormationTaskPlugin::analyze()
{
    RoguelikeFormationImageAnalyzer formation_analyzer(ctrler()->get_image());
    if (!formation_analyzer.analyze()) {
        return false;
    }

    auto oper_result = formation_analyzer.get_result();
    bool added = false;
    for (auto& oper : oper_result) {
        bool already_exist = false;
        for (const auto& existing_oper : oper_list) {
            // 这里其实没考虑存在多个相同预备干员的情况，不过影响应该不大？
            if (oper.name == existing_oper.name) already_exist = true;
        }
        Log.debug(__FUNCTION__, "oper: ", oper.name, " page: ", cur_page, " already_exist: ", already_exist);
        if (!already_exist) {
            oper.page = cur_page;
            oper_list.emplace_back(oper);
            added = true; // 希望OCR没识别错干员名，否则可能误判当前页面的干员情况
        }
    }
    return added;
}

bool asst::RoguelikeFormationTaskPlugin::select(asst::RoguelikeFormationImageAnalyzer::FormationOper oper)
{
    if (cur_page != oper.page) {
        // 在最大页码时（仅当总页数>=3），从右往左划可能会有对不齐的问题，直接划动到底
        if (cur_page > oper.page && max_page >= 3 && cur_page == max_page) {
            for (; cur_page > 0; --cur_page) { // 多划一次到不存在的第0页
                ProcessTask(*this, { "RoguelikeRecruitOperListSwipeToTheLeft" }).run();
            }
            ProcessTask(*this, { "SleepAfterOperListQuickSwipe" }).run();
            cur_page = 1;
        }
        // 中间页面划动姑且当成是相对准确的
        while (cur_page > oper.page) {
            ProcessTask(*this, { "RoguelikeRecruitOperListSlowlySwipeToTheLeft" }).run();
            cur_page--;
        }
        while (cur_page < oper.page) {
            ProcessTask(*this, { "RoguelikeRecruitOperListSlowlySwipeToTheRight" }).run();
            cur_page++;
        }
    }
    ctrler()->click(oper.rect);
    return true;
}
