#include "InfrastTradeTask.h"

#include "Utils/Logger.hpp"

bool asst::InfrastTradeTask::_run()
{
    m_all_available_opers.clear();

    // rooms 数组为空时整设施保持原样；
    // skip 与空意图不在设施入口判定，分别留在循环内 Pre 无人机后与产物处理后按时序消费
    if (is_facility_config_empty() && check_room_shift_intent() == RoomShiftIntent::KeepUnchanged) {
        return true;
    }

    swipe_to_the_left_of_main_ui();
    // 断点续跑：任务重试轮从失败的房间接着跑（m_cur_facility_index 停在失败间），
    // 已成功房间不再重走，编组可用集的消耗状态因此天然正确；首轮从第 0 间开始
    enter_facility(m_cur_retry > 0 ? m_cur_facility_index : 0);
    click_bottom_left_tab();

    if (!shift_facility_list()) {
        return false;
    }

    return true;
}
