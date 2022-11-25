#include "InfrastTradeTask.h"

bool asst::InfrastTradeTask::_run()
{
    m_all_available_opers.clear();

    swipe_to_the_left_of_main_ui();
    enter_facility(0);
    click_bottom_left_tab();

    if (!shift_facility_list()) {
        return false;
    }

    return true;
}
