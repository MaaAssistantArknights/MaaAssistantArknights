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
    if (msg != AsstMsg::SubTaskCompleted
        || details.get("subtask", std::string()) != "ProcessTask") {
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
    if (task_view == "Roguelike@QuickFormation") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeFormationTaskPlugin::_run()
{
    LogTraceFunction;

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
        size_t new_selected_count =
            ranges::count_if(new_result, [](const auto& oper) { return oper.selected; });
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
    LogTraceFunction;

    // 清空并退出游戏会自动按等级重新排序
    ProcessTask(*this, { "RoguelikeQuickFormationClearAndReselect" }).run();

    m_last_detected_oper_names.clear();
    oper_list.clear();
    cur_page = 1;

    while (analyze()) { // 返回true说明新增了干员，可能还有下一页
        ProcessTask(*this, { "RoguelikeRecruitOperListSlowlySwipeToTheRight" }).run();
        cur_page++;
    }

    cur_page--; // 最后多划了一下，退回最后一页的页码
    max_page = cur_page;

    // 将oper_list的每个干员重置为未选择
    for (auto& oper : oper_list) {
        oper.selected = false;
    }

    Log.info(__FUNCTION__, "max_page: ", max_page, " oper_count: ", oper_list.size());

    std::vector<asst::RoguelikeFormationImageAnalyzer::FormationOper> sorted_oper_list;
    std::unordered_set<std::string> oper_to_select; // 和上面的 vector 一致，用于快速查重

    const auto& team_complete_condition =
        RoguelikeRecruit.get_team_complete_info(m_config->get_theme());
    const auto& group_list = RoguelikeRecruit.get_group_info(m_config->get_theme());

    for (const auto& condition : team_complete_condition) { // 优先选择阵容核心干员
        int count = 0;
        const int require = condition.threshold;
        for (const std::string& group_name : condition.groups) {
            auto group_filter = views::filter([&](const auto& oper) {
                const auto& group_ids =
                    RoguelikeRecruit.get_group_id(m_config->get_theme(), oper.name);
                return ranges::any_of(group_ids, [&](int id) {
                    return group_list[id] == group_name;
                });
            });
            for (const auto& oper : oper_list | group_filter | views::take(require - count)) {
                if (!oper_to_select.contains(oper.name)) {
                    sorted_oper_list.emplace_back(oper);
                    oper_to_select.emplace(oper.name);
                }
                count++;
            }
            if (count == require) {
                break;
            }
        }
    }

    std::erase_if(oper_list, [&](const auto& oper) { return oper_to_select.contains(oper.name); });
    ranges::stable_partition(oper_list, [](const auto& oper) {
        return !oper.name.starts_with("预备干员");
    });
    ranges::move(
        oper_list | views::take(13 - sorted_oper_list.size()),
        std::back_inserter(sorted_oper_list));

    for (const auto& oper : sorted_oper_list) {
        select(oper);
    }
}

bool asst::RoguelikeFormationTaskPlugin::analyze()
{
    LogTraceFunction;

    RoguelikeFormationImageAnalyzer formation_analyzer(ctrler()->get_image());
    if (!formation_analyzer.analyze()) {
        return false;
    }

    // 比较在新一页识别到的干员名 （detected_oper_names） 与 上一页识别到的干员名 (m_last_detected_oper_names)
    // 若完全相同则认为已到达尾页
    auto formation_analyze_result = formation_analyzer.get_result();
    auto oper_name_view = formation_analyze_result | views::transform([&](auto oper) { return oper.name; });
    std::vector<std::string> detected_oper_names(oper_name_view.begin(), oper_name_view.end());
    const bool reach_last_column = (detected_oper_names == m_last_detected_oper_names);
    m_last_detected_oper_names = std::move(detected_oper_names);

    auto unique_filter = views::filter([&](const auto& oper) {
        // TODO: 这里没考虑多个相同预备干员的情况，不过影响应该不大
        return !ranges::any_of(oper_list, [&](const auto& existing_oper) {
            return oper.name == existing_oper.name;
        });
    });
    auto append_page_proj = views::transform([&](auto oper) {
        Log.info(__FUNCTION__, "oper: ", oper.name, " page: ", cur_page);
        oper.page = cur_page;
        return oper;
    });
    auto result_oper_list = formation_analyze_result | unique_filter | append_page_proj;
    ranges::move(result_oper_list, std::back_inserter(oper_list));
    return reach_last_column;
}

bool asst::RoguelikeFormationTaskPlugin::select(RoguelikeFormationImageAnalyzer::FormationOper oper)
{
    LogTraceFunction;

    if (cur_page != oper.page) {
        Log.info(__FUNCTION__, "swipe from page", cur_page, "to page", oper.page);
        // 在最大页码时（当总页数>=2），从右往左划可能会有对不齐的问题，直接划动到底
        if ((cur_page > oper.page && max_page >= 2 && cur_page == max_page) || max_page == 1) {
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
    Log.info(__FUNCTION__, "select oper", oper.name, "on page", cur_page);
    ctrler()->click(oper.rect);
    return true;
}
