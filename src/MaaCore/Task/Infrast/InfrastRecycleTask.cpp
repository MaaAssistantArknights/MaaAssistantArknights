#include "InfrastRecycleTask.h"

#include "Controller/Controller.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"

namespace
{
// 防御滑动上限，避免识别异常时在干员列表里无限翻页
constexpr int MaxSwipeTimes = 5;
} // namespace

bool asst::InfrastRecycleTask::_run()
{
    // 回收站只有一个，无产物概念
    set_product("Placeholder");
    swipe_to_the_left_of_main_ui();
    if (!enter_facility()) {
        swipe_to_right_of_main_ui();
        if (!enter_facility()) {
            return false;
        }
    }

    if (!enter_oper_list_page()) {
        return false;
    }

    close_quick_formation_expand_role();

    // 清空后按位置补满，不挑选技能
    click_clear_button();
    if (!fill_slots()) {
        return false;
    }

    click_confirm_button();
    click_return_button();

    return true;
}

bool asst::InfrastRecycleTask::fill_slots()
{
    LogTraceFunction;

    // 清空后没有已选干员，直接按位置取列表前 max_num_of_opers() 个补位。
    // 正在其他设施工作的干员（卡片带"工作中"标记）不拉取，避免拆散已换班的其他设施。
    size_t num_of_selected = 0;
    int swipe_times = 0;
    while (num_of_selected < max_num_of_opers()) {
        if (need_exit()) {
            return false;
        }
        const auto image = ctrler()->get_image();
        InfrastOperImageAnalyzer oper_analyzer(image);
        constexpr int without_skill = InfrastOperImageAnalyzer::All ^ InfrastOperImageAnalyzer::Skill;
        oper_analyzer.set_to_be_calced(without_skill);
        if (!oper_analyzer.analyze()) {
            Log.error("recycle oper analyze failed");
            return false;
        }
        oper_analyzer.sort_by_loc();

        for (const auto& oper : oper_analyzer.get_result()) {
            if (num_of_selected >= max_num_of_opers()) {
                break;
            }
            if (oper.selected || oper.doing == infrast::Doing::Working) {
                continue;
            }
            ctrler()->click(oper.rect);
            ++num_of_selected;
        }
        if (num_of_selected >= max_num_of_opers()) {
            break;
        }
        if (++swipe_times > MaxSwipeTimes) {
            Log.warn("recycle fill slots swiped too many times, give up", num_of_selected);
            break;
        }
        // 识别完了还要点击，所以这里不能异步滑动
        swipe_of_operlist();
    }
    if (num_of_selected < max_num_of_opers()) {
        callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("NotEnoughStaff"));
    }
    if (swipe_times > 0) {
        swipe_to_the_left_of_operlist(swipe_times + 1);
    }
    return true;
}
