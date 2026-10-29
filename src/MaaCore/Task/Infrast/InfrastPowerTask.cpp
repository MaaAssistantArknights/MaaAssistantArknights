#include "InfrastPowerTask.h"

#include "Controller/Controller.h"
#include <ranges>

bool asst::InfrastPowerTask::_run()
{
    m_all_available_opers.clear();

    // 发电站只能造这一个
    set_product("Drone");

    // rooms 数组为空时整设施保持原样；
    // skip 与空意图不在设施入口判定，留在循环内进设施前按时序消费
    if (is_facility_config_empty() && check_room_shift_intent() == RoomShiftIntent::KeepUnchanged) {
        return true;
    }

    for (; m_cur_facility_index != static_cast<int>(max_num_of_facilities()); ++m_cur_facility_index) {
        if (need_exit()) {
            return false;
        }
        // rooms 数组短于实际房间数：剩余房间不换班（与制造站贸易站及文档语义一致），整设施发一条未配置提示
        if (m_is_custom && static_cast<size_t>(m_cur_facility_index) >= m_custom_config.size()) {
            notify_rooms_not_configured();
            break;
        }
        // 空数组已在此前消费，此处结果只可能 Skip、KeepUnchanged（空意图）或 Shift（数组短的越界房间已在上游 break）
        if (check_room_shift_intent() != RoomShiftIntent::Shift) {
            continue;
        }

        swipe_to_the_left_of_main_ui();

        // 进不去说明设施数量不够
        if (!enter_facility(m_cur_facility_index, is_index_within_recognized_count(m_cur_facility_index))) {
            break;
        }

        if (!enter_oper_list_page()) {
            return false;
        }

        close_quick_formation_expand_role();

        bool room_kept_unchanged = false;
        for (int j = 0; j <= OperSelectRetryTimes; ++j) {
            if (is_use_custom_opers()) {
                if (swipe_and_select_custom_opers(false, false, &room_kept_unchanged)) {
                    break;
                }
                if (room_kept_unchanged) {
                    // 保持原样已由选人函数退出选人界面（停在房间页），此处补齐一次返回到主界面，
                    // 对齐确认路径的退出深度并跳过确认（循环后的确认是纯坐标点击，不识别界面）；
                    // 下一间开头的滑动与 enter_facility 要求停留在主界面
                    click_return_button();
                    break;
                }
                swipe_to_the_left_of_operlist();
                continue;
            }

            // 常规算法从包括在班干员在内的完整列表里挑人，先把筛选恢复为“全部”
            if (!ensure_notstationed_filter_off()) {
                return false;
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
        if (room_kept_unchanged) {
            continue;
        }
        click_confirm_button();
        click_return_button();
    }

    return true;
}
