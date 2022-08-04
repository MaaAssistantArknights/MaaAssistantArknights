#include "InfrastPowerTask.h"

#include "Controller.h"

#include "AsstRanges.hpp"

bool asst::InfrastPowerTask::_run()
{
    m_all_available_opers.clear();

    // 发电站只能造这一个
    set_product("Drone");

    for (; m_cur_facility_index != static_cast<int>(max_num_of_facilities()); ++m_cur_facility_index) {
        if (need_exit()) {
            return false;
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
            if (m_all_available_opers.empty()) {
                if (!opers_detect_with_swipe()) {
                    return false;
                }
                swipe_to_the_left_of_operlist();
            }
            else {
                opers_detect();
            }

            auto find_iter = ranges::find_if(m_all_available_opers,
                [&](const infrast::Oper& info) -> bool {
                    return info.selected;
                });
            bool need_shift = true;
            if (find_iter != m_all_available_opers.end()) {
                switch (m_work_mode) {
                case infrast::WorkMode::Gentle:
                    // 如果之前有干员在，那就不换人，直接退出
                    m_all_available_opers.erase(find_iter);
                    need_shift = false;
                    break;
                case infrast::WorkMode::Aggressive:
                    need_shift = true;
                    // TODO，这里有个bug，全部干员中的selected，和当前的，不一定是同一个页面
                    // 不过目前没影响，反正滑动到最前面了，selected的一定是在最前面
                    //m_ctrler->click(find_iter->rect);
                    //sleep(300);
                    break;
                case infrast::WorkMode::Extreme: // TODO
                    break;
                default:
                    break;
                }
            }
            if (need_shift) {
                optimal_calc();
                if (!opers_choose()) {
                    m_all_available_opers.clear();
                    swipe_to_the_left_of_operlist();
                    continue;
                }
            }
            break;
        }
        click_confirm_button();
        click_return_button();
    }

    return true;
}
