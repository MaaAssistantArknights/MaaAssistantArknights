#include "InfrastControlTask.h"

bool asst::InfrastControlTask::_run()
{
    m_all_available_opers.clear();

    // 控制中枢只能造这一个
    set_product("MoodAddition");

    if (!enter_facility()) {
        return false;
    }
    if (!enter_oper_list_page()) {
        return false;
    }

    for (int i = 0; i <= OperSelectRetryTimes; ++i) {
        if (need_exit()) {
            return false;
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
