#include "InfrastOfficeTask.h"

#include "AsstRanges.hpp"
#include "Controller.h"
#include "Logger.hpp"

bool asst::InfrastOfficeTask::_run()
{
    m_all_available_opers.clear();

    // 办公室只能造这一个
    set_product("HR");
    if (is_use_custom_config() && m_current_room_custom_config.skip) {
        Log.info("skip this room");
        return true;
    }
    swipe_to_the_right_of_main_ui();
    enter_facility();
    click_bottom_left_tab();

    for (int i = 0; i <= OperSelectRetryTimes; ++i) {
        if (need_exit()) {
            return false;
        }
        if (is_use_custom_config()) {
            bool name_select_ret = swipe_and_select_custom_opers();
            if (name_select_ret) {
                break;
            }
            else {
                swipe_to_the_left_of_operlist();
                continue;
            }
        }

        if (!opers_detect_with_swipe()) {
            return false;
        }
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
