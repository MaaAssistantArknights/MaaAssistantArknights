#include "InfrastControlTask.h"

#include <functional>
#include <ranges>

#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"

bool asst::InfrastControlTask::_run()
{
    m_all_available_opers.clear();

    // 控制中枢只能造这一个
    set_product("MoodAddition");
    if (m_is_custom && current_room_config().skip) {
        Log.info("skip this room");
        return true;
    }
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

    if (m_vacancy_only) {
        InfrastOperImageAnalyzer stationed_analyzer(ctrler()->get_image());
        stationed_analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::All);
        stationed_analyzer.set_facility(facility_name());
        if (!stationed_analyzer.analyze()) {
            return false;
        }
        m_cur_num_of_locked_opers = static_cast<int>(
            std::ranges::count_if(stationed_analyzer.get_result(), std::mem_fn(&infrast::Oper::selected)));
        if (m_cur_num_of_locked_opers >= static_cast<int>(max_num_of_opers())) {
            Log.info("control center has no vacancy, skip second selection");
            click_return_button();
            return true;
        }
    }

    // 如果是使用了编队组来排班
    if (current_room_config().use_operator_groups) {
        match_operator_groups();
    }

    bool selection_ready = false;
    for (int i = 0; i <= OperSelectRetryTimes; ++i) {
        if (need_exit()) {
            return false;
        }
        if (is_use_custom_opers()) {
            bool name_select_ret = swipe_and_select_custom_opers();
            if (name_select_ret) {
                break;
            }
            else {
                swipe_to_the_left_of_operlist();
                continue;
            }
        }

        if (m_default_mode) {
            // 按技能排序后，同设施技能的干员会连续排列。否则前序设施留下的工作状态排序
            // 可能让正在宿舍休息的候选落到无关技能之后，扫描会在中间空页提前结束。
            ProcessTask(*this, { "InfrastOperListTabSkillUnClicked", "Stop" }).run();
        }
        if (!opers_detect_with_swipe()) {
            return false;
        }
        swipe_to_the_left_of_operlist();

        if (m_vacancy_only) {
            std::erase_if(m_all_available_opers, std::mem_fn(&infrast::Oper::selected));
        }

        optimal_calc();

        // 清空按钮放到识别完之后，现在通过切换职业栏来回到界面最左侧，先清空会导致当前设施里的人排到最后面
        if (!m_vacancy_only) {
            click_clear_button();
        }
        if (!opers_choose()) {
            m_all_available_opers.clear();
            swipe_to_the_left_of_operlist();
            continue;
        }
        selection_ready = true;
        break;
    }
    if (!selection_ready) {
        discard_pending_selection();
        return false;
    }
    if (!click_confirm_button()) {
        return false;
    }
    click_return_button();

    return true;
}
