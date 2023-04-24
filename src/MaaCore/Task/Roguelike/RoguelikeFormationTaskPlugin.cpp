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
    first_page_full = false;

    if (pre_selected == MaxNumOfOperPerPage) {
        first_page_full = true;
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
    analyze();

    if (first_page_full) { // 说明第二页还有
        ProcessTask(*this, { "RoguelikeRecruitOperListSlowlySwipeToTheRight" }).run();
        cur_page = 2;
        analyze();
        // 应该不可能还有第三页吧，不管了
    }

    std::vector<asst::RoguelikeFormationImageAnalyzer::FormationOper> sorted_oper_list;
    const std::string rogue_theme = status()->get_properties(Status::RoguelikeTheme).value();
    const auto& team_complete_condition = RoguelikeRecruit.get_team_complete_info(rogue_theme);
    const auto& group_list = RoguelikeRecruit.get_group_info(rogue_theme);
    for (const auto& condition : team_complete_condition) { // 优先选择阵容核心干员
        int count = 0;
        int require = condition.threshold;
        for (const std::string& group_name : condition.groups) {
            for (const auto& oper : oper_list) {
                int group_id = RoguelikeRecruit.get_group_id(rogue_theme, oper.name);
                std::string oper_group = group_list[group_id];
                if (oper_group == group_name) {
                    sorted_oper_list.emplace_back(oper);
                    count++;
                }
                if (count == require) break;
            }
            if (count == require) break;
        }
    }
    for (const auto& oper : oper_list) { // 然后按默认排序选择
        if (sorted_oper_list.size() >= 13) break;
        bool already_exist = false;
        for (const auto& existing_oper : sorted_oper_list) {
            if (oper.name == existing_oper.name) already_exist = true;
        }
        if (!already_exist) {
            sorted_oper_list.emplace_back(oper);
        }
    }
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
    for (auto& oper : oper_result) {
        bool already_exist = false;
        for (const auto& existing_oper : oper_list) {
            if (oper.name == existing_oper.name) already_exist = true;
        }
        if (!already_exist) {
            oper.page = cur_page;
            oper_list.emplace_back(oper);
        }
    }
    return true;
}

bool asst::RoguelikeFormationTaskPlugin::select(asst::RoguelikeFormationImageAnalyzer::FormationOper oper)
{
    if (cur_page != oper.page) {
        if (cur_page == 1) {
            ProcessTask(*this, { "RoguelikeRecruitOperListSlowlySwipeToTheRight" }).run();
            cur_page = 2;
        }
        else {
            ProcessTask(*this, { "RoguelikeRecruitOperListSlowlySwipeToTheLeft" }).run();
            cur_page = 1;
        }
    }
    ctrler()->click(oper.rect);
    return true;
}
