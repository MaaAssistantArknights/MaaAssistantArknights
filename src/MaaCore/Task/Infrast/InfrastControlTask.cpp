#include "InfrastControlTask.h"

#include "Utils/Logger.hpp"

bool asst::InfrastControlTask::_run()
{
    m_all_available_opers.clear();

    // 控制中枢只能造这一个
    set_product("MoodAddition");
    if (m_is_custom && current_room_config().skip) {
        Log.info("skip this room");
        return true;
    }
    if (!enter_facility()) {
        return false;
    }
    if (!enter_oper_list_page()) {
        return false;
    }

    // 如果是使用了编队组来排班
    if (current_room_config().use_operator_groups) {
        match_operator_groups();
    }

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

        click_clear_button();

        if (!opers_detect_with_swipe()) {
            return false;
        }
        swipe_to_the_left_of_operlist();

        optimal_calc();
        if (!opers_choose()) {
            m_all_available_opers.clear();
            swipe_to_the_left_of_operlist();
            continue;
        }
        break;
    }
    click_confirm_button();
    click_return_button();

    return true;
}
