#include "InfrastControlTask.h"

const std::string asst::InfrastControlTask::FacilityName = "Control";

bool asst::InfrastControlTask::_run()
{
    json::value task_start_json = json::object{
        { "task_type", "InfrastControlTask" },
        { "task_chain", m_task_chain }
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    set_facility(FacilityName);
    m_all_available_opers.clear();

    // 控制中枢只能造这一个
    set_product("MoodAddition");

    // 进不去说明设施数量不够
    if (!enter_facility(FacilityName, 0)) {
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
        bool ret = opers_choose();
        if (!ret) {
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
