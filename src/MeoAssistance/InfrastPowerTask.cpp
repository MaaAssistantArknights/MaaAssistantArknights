#include "InfrastPowerTask.h"

const std::string asst::InfrastPowerTask::FacilityName = "Power";

bool asst::InfrastPowerTask::run()
{
    json::value task_start_json = json::object{
        { "task_type",  "InfrastPowerTask" },
        { "task_chain", m_task_chain}
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    set_facility(FacilityName);
    m_all_available_opers.clear();

    swipe_to_the_left_of_main_ui();

    for (int i = 0; i != 3; ++i) {
        enter_facility(FacilityName, i);

        if (!enter_oper_list_page()) {
            return false;
        }

        // 发电站只能造这一个
        set_product("Drone");
        /* 进入干员选择页面 */
        click_clear_button();
        swipe_to_the_left_of_operlist();

        if (m_all_available_opers.empty()) {
            opers_detect_with_swipe();
            swipe_to_the_left_of_operlist();
        }
        else {
            opers_detect();
        }
        optimal_calc();
        opers_choose();
        click_confirm_button();
    }

    return true;
}