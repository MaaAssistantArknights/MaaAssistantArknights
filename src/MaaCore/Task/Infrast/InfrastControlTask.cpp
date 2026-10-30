#include "InfrastControlTask.h"

#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"

bool asst::InfrastControlTask::_run()
{
    m_all_available_opers.clear();

    // 拦截统一走基类 check_room_shift_intent，非 Shift 视为本房间处理完成
    if (check_room_shift_intent() != RoomShiftIntent::Shift) {
        return true;
    }

    // 控制中枢只能造这一个
    set_product("MoodAddition");

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

    // 如果是使用了编队组来排班
    // 全部编组不可用时，候补名单非空则照常走候补选人，仅无候补且无 autofill 时保持房间原样，
    // 退出选人界面并视为完成，不回落默认算法自动选人，避免拆走后续设施点名/编组指定的干员；
    // 识别失败不是排班计划内的降级，不启用候补，同样保持房间原样并上报原因；
    // need_exit 场景留给选人循环开头的既有检查中止
    if (current_room_config().use_operator_groups) {
        const GroupMatchResult group_match_result = match_operator_groups();
        if (group_match_result == GroupMatchResult::AllUnavailable && current_room_config().candidates.empty() &&
            !current_room_config().autofill && !need_exit()) {
            LogInfo << "all operator groups unavailable with no candidates or autofill, keep room unchanged";
            click_return_button();
            return true;
        }
        if (group_match_result == GroupMatchResult::RecogFailed && !need_exit()) {
            LogInfo << "failed to recognize operators for group matching, keep room unchanged";
            json::value info = basic_info_with_what("CustomInfrastRoomGroupsMatchFailed");
            info["details"]["reason"] = "recognition_failed";
            callback(AsstMsg::SubTaskExtraInfo, info);
            click_return_button();
            return true;
        }
    }

    bool selection_ready = false;
    for (int i = 0; i <= OperSelectRetryTimes; ++i) {
        if (need_exit()) {
            return false;
        }
        if (is_use_custom_opers()) {
            bool room_kept_unchanged = false;
            bool name_select_ret = swipe_and_select_custom_opers(false, false, &room_kept_unchanged);
            if (name_select_ret) {
                selection_ready = true;
                break;
            }
            if (room_kept_unchanged) {
                // 保持原样已由选人函数退出选人界面（停在房间页），此处补齐一次返回到主界面，
                // 对齐确认路径的退出深度，并跳过确认与剩余重试
                click_return_button();
                return true;
            }
            swipe_to_the_left_of_operlist();
            continue;
        }

        if (m_default_mode) {
            // 按技能排序后，同设施技能的干员会连续排列。否则前序设施留下的工作状态排序
            // 可能让正在宿舍休息的候选落到无关技能之后，扫描会在中间空页提前结束。
            ProcessTask(*this, { "InfrastOperListTabSkillUnClicked", "Stop" }).run();
        }
        // 常规算法从包括在班干员在内的完整列表里挑人，先把筛选恢复为“全部”
        if (!ensure_notstationed_filter_off()) {
            return false;
        }
        if (!opers_detect_with_swipe()) {
            return false;
        }
        swipe_to_the_left_of_operlist();

        optimal_calc();

        // 清空按钮放到识别完之后，现在通过切换职业栏来回到界面最左侧，先清空会导致当前设施里的人排到最后面
        click_clear_button();
        if (!opers_choose()) {
            m_all_available_opers.clear();
            swipe_to_the_left_of_operlist();
            continue;
        }
        selection_ready = true;
        break;
    }
    if (!selection_ready) {
        restore_last_matched_group_opers();
        discard_pending_selection();
        return false;
    }
    if (!click_confirm_button()) {
        // 选人未生效即失败，编组可用集回补，重试轮该组仍可重新匹配
        restore_last_matched_group_opers();
        return false;
    }
    m_last_matched_group_opers.clear();
    click_return_button();

    return true;
}
