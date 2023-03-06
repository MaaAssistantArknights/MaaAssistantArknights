#include "InfrastPowerTask.h"

#include "Controller/Controller.h"
#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"

bool asst::InfrastPowerTask::_run()
{
    m_all_available_opers.clear();

    // 发电站只能造这一个
    set_product("Drone");

    for (; m_cur_facility_index != static_cast<int>(max_num_of_facilities()); ++m_cur_facility_index) {
        if (need_exit()) {
            return false;
        }
        if (m_is_custom && current_room_config().skip) {
            Log.info("skip this room");
            continue;
        }
        swipe_to_the_left_of_main_ui();

        // 进不去说明设施数量不够
        if (!enter_facility(m_cur_facility_index)) {
            break;
        }
        if (!enter_oper_list_page()) {
            return false;
        }

        for (int j = 0; j <= OperSelectRetryTimes; ++j) {
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

            if (m_all_available_opers.empty()) {
                if (!opers_detect_with_swipe()) {
                    return false;
                }
                swipe_to_the_left_of_operlist();
            }
            else {
                opers_detect();
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
    }

    return true;
}
