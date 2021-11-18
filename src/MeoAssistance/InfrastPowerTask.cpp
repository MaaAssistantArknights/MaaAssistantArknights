#include "InfrastPowerTask.h"

#include "Controller.h"

const std::string asst::InfrastPowerTask::FacilityName = "Power";

bool asst::InfrastPowerTask::run()
{
    json::value task_start_json = json::object{
        { "task_type", "InfrastPowerTask" },
        { "task_chain", m_task_chain }
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    set_facility(FacilityName);
    m_all_available_opers.clear();

    // 发电站只能造这一个
    set_product("Drone");

    for (int i = 0; i != MaxNumOfFacility; ++i) {
        if (need_exit()) {
            return false;
        }
        swipe_to_the_left_of_main_ui();

        // 进不去说明设施数量不够
        if (!enter_facility(FacilityName, i)) {
            break;
        }
        if (!enter_oper_list_page()) {
            return false;
        }
        constexpr int retry_times = 1;
        for (int i = 0; i <= retry_times; ++i) {
            swipe_to_the_left_of_operlist();

            if (m_all_available_opers.empty()) {
                opers_detect_with_swipe();
                swipe_to_the_left_of_operlist();
            }
            else {
                opers_detect();
            }

            auto find_iter = std::find_if(
                m_all_available_opers.begin(), m_all_available_opers.end(),
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
                    ctrler.click(find_iter->rect);
                    break;
                case infrast::WorkMode::Extreme: // TODO
                    break;
                default:
                    break;
                }
            }
            if (need_shift) {
                optimal_calc();
                bool ret = opers_choose();
                if (!ret) {
                    m_all_available_opers.clear();
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
