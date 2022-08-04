#include "RoguelikeFormationTaskPlugin.h"

#include "AsstRanges.hpp"

#include "RoguelikeFormationImageAnalyzer.h"
#include "Controller.h"
#include "TaskData.h"
#include "ProcessTask.h"
#include "Logger.hpp"

bool asst::RoguelikeFormationTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted
        || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "Roguelike1QuickFormation") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeFormationTaskPlugin::_run()
{
    RoguelikeFormationImageAnalyzer formation_analyzer(m_ctrler->get_image());
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
        m_ctrler->click(oper.rect);
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
        sleep(Task.get("Roguelike1QuickFormationDelay")->rear_delay);
        formation_analyzer.set_image(m_ctrler->get_image());

        if (!formation_analyzer.analyze()) {
            Log.warn("RoguelikeFormationImageAnalyzer re analyze failed");
            return true;
        }

        auto& new_result = formation_analyzer.get_result();
        size_t new_selected_count = ranges::count_if(new_result,
            [](const auto& oper) { return oper.selected; });
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
    ProcessTask(*this, { "Roguelike1QuickFormationClearAndReselect" }).run();

    size_t select_count = analyze_and_select();
    // 说明第二页还有
    if (select_count == MaxNumOfOperPerPage) {
        ProcessTask(*this, { "SlowlySwipeToTheRight" }).run();
        analyze_and_select();
        // 应该不可能还有第三页吧，不管了
    }
}

size_t asst::RoguelikeFormationTaskPlugin::analyze_and_select()
{
    RoguelikeFormationImageAnalyzer formation_analyzer(m_ctrler->get_image());
    if (!formation_analyzer.analyze()) {
        return false;
    }

    size_t select_count = 0;
    for (const auto& oper : formation_analyzer.get_result()) {
        if (oper.selected) {
            continue;
        }
        m_ctrler->click(oper.rect);
        ++select_count;
    }
    return select_count;
}
