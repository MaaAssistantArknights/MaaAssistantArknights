#include "InfrastTradeTask.h"

#include "Utils/Logger.hpp"

bool asst::InfrastTradeTask::_run()
{
    m_all_available_opers.clear();

    if (is_facility_config_empty()) {
        LogInfo << "custom room array is empty, keep facility unchanged";
        return true;
    }

    swipe_to_the_left_of_main_ui();
    enter_facility(0);
    click_bottom_left_tab();

    if (!shift_facility_list()) {
        return false;
    }

    return true;
}
