#include "InfrastOfficeTask.h"

#include "Controller/Controller.h"
#include <ranges>

bool asst::InfrastOfficeTask::_run()
{
    m_all_available_opers.clear();

    // 拦截统一走基类 check_room_shift_intent，非 Shift 视为本房间处理完成
    if (check_room_shift_intent() != RoomShiftIntent::Shift) {
        return true;
    }

    // 办公室只能造这一个
    set_product("HR");

    swipe_to_the_left_of_main_ui();
    if (!enter_facility(0, false)) {
        swipe_to_right_of_main_ui();
        if (!enter_facility()) {
            return false;
        }
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
            bool room_kept_unchanged = false;
            bool name_select_ret = swipe_and_select_custom_opers(false, false, &room_kept_unchanged);
            if (name_select_ret) {
                break;
            }
            if (room_kept_unchanged) {
                // 保持原样已由选人函数退出选人界面（停在房间页），此处补齐一次返回到主界面，
                // 对齐确认路径的退出深度，并跳过确认与剩余重试（循环后的确认是纯坐标点击，不识别界面）
                click_return_button();
                return true;
            }
            swipe_to_the_left_of_operlist();
            continue;
        }

        // 常规算法从包括在班干员在内的完整列表里挑人，先把筛选恢复为“全部”
        if (!ensure_notstationed_filter_off()) {
            return false;
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
