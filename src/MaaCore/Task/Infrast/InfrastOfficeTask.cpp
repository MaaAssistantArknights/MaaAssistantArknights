#include "InfrastOfficeTask.h"

#include "Controller/Controller.h"
#include "Utils/Logger.hpp"
#include <ranges>

bool asst::InfrastOfficeTask::_run()
{
    m_all_available_opers.clear();

    // 办公室只能造这一个
    set_product("HR");
    if (m_is_custom && current_room_config().skip) {
        Log.info("skip this room");
        return true;
    }
    if (is_facility_config_empty()) {
        LogInfo << "custom room array is empty, keep facility unchanged";
        return true;
    }
    swipe_to_the_left_of_main_ui();
    if (!enter_facility()) {
        swipe_to_right_of_main_ui();
        if (!enter_facility()) {
            return false;
        }
    }

    // 房间对象存在但没有给出任何选人意图（operators/candidates 为空或缺失、未配置 autofill 与编组引用）时
    // 保持房间原样，不回落默认算法自动选人
    if (is_room_config_specified() && !current_room_config().skip && current_room_config().names.empty() &&
        current_room_config().candidates.empty() && !current_room_config().autofill) {
        LogInfo << "room config exists but no operator selection intent, keep room unchanged";
        json::value info = basic_info_with_what("CustomInfrastRoomKeptUnchanged");
        callback(AsstMsg::SubTaskExtraInfo, info);
        return true;
    }

    if (!enter_oper_list_page()) {
        return false;
    }

    close_quick_formation_expand_role();

    for (int i = 0; i <= OperSelectRetryTimes; ++i) {
        if (need_exit()) {
            return false;
        }
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
