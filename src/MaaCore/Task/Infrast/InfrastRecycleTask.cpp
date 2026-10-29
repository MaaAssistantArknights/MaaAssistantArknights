#include "InfrastRecycleTask.h"

#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"

// 防御滑动上限，避免识别异常时在干员列表里无限翻页
static constexpr int MaxSwipeTimes = 5;

bool asst::InfrastRecycleTask::_run()
{
    // 拦截统一走基类 check_room_shift_intent，非 Shift 视为本房间处理完成
    if (check_room_shift_intent() != RoomShiftIntent::Shift) {
        return true;
    }

    // 回收站只有一个，无产物概念
    set_product("Placeholder");

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

    // 全部编组不可用时，候补名单非空则照常走候补选人，仅无候补且无 autofill 时保持房间原样，
    // 不回落默认算法自动选人，避免拆走后续设施点名/编组指定的干员；
    // 识别失败不是排班计划内的降级，不启用候补，同样保持房间原样并上报原因；
    // need_exit 场景留给选人循环开头的既有检查中止。
    // 下方按位置补满的常规路径仅服务默认模式与 autofill 授权的房间
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

    for (int i = 0; i <= OperSelectRetryTimes; ++i) {
        if (need_exit()) {
            return false;
        }
        if (is_use_custom_opers()) {
            bool room_kept_unchanged = false;
            if (swipe_and_select_custom_opers(false, false, &room_kept_unchanged)) {
                if (!click_confirm_button()) {
                    // 选人未生效即失败，编组可用集回补，重试轮该组仍可重新匹配
                    restore_last_matched_group_opers();
                    return false;
                }
                m_last_matched_group_opers.clear();
                click_return_button();
                return true;
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

        // 清空前现任仍处于选中置顶，先采样本设施编号供补位排除豁免（连任候选）；
        // 清空后置顶机制失效，采样不再可靠
        detect_current_facility_number();
        // 清空后按位置补满，不挑选技能
        click_clear_button();
        if (!fill_slots()) {
            return false;
        }

        click_confirm_button();
        click_return_button();
        return true;
    }

    LogWarn << "recycle custom shift failed after retries";
    restore_last_matched_group_opers();
    click_return_button();
    return false;
}

bool asst::InfrastRecycleTask::fill_slots()
{
    LogTraceFunction;

    // 固定按工作状态排序，避免沿用上一设施或用户手动残留的排序导致选人不可预测
    ProcessTask(*this, { "InfrastOperListTabWorkStatusUnClicked", "Stop" }).run();

    // 驻扎生产设施的干员（编号判定）不拉取，避免拆散已换班的其他设施；宿舍编号与
    // 未进驻不排除。心情低于阈值的干员跳过，避免打断宿舍休息循环；工作中标记的
    // 跳过保留为防御（编号 OCR 漏读时兜底）
    size_t num_of_selected = 0;
    int swipe_times = 0;
    while (num_of_selected < max_num_of_opers()) {
        if (need_exit()) {
            return false;
        }
        const auto image = ctrler()->get_image();
        InfrastOperImageAnalyzer oper_analyzer(image);
        constexpr int calced =
            InfrastOperImageAnalyzer::Selected | InfrastOperImageAnalyzer::Doing | InfrastOperImageAnalyzer::Mood;
        oper_analyzer.set_to_be_calced(calced);
        if (!oper_analyzer.analyze()) {
            LogError << "recycle oper analyze failed";
            return false;
        }
        oper_analyzer.sort_by_loc();

        for (const auto& oper : oper_analyzer.get_result()) {
            if (num_of_selected >= max_num_of_opers()) {
                break;
            }
            // 编号 OCR 每卡一次开销最大，放在内存已算好的判断之后兜底
            if (oper.selected || oper.doing == infrast::Doing::Working || oper.mood_ratio < m_mood_threshold ||
                stationed_in_production_facility(oper)) {
                continue;
            }
            ctrler()->click(oper.rect);
            ++num_of_selected;
        }
        if (num_of_selected >= max_num_of_opers()) {
            break;
        }
        if (++swipe_times > MaxSwipeTimes) {
            LogWarn << "recycle fill slots swiped too many times, give up" << num_of_selected;
            break;
        }
        // 识别完了还要点击，所以这里不能异步滑动
        swipe_of_operlist();
    }
    if (num_of_selected < max_num_of_opers()) {
        callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("NotEnoughStaff"));
    }
    if (swipe_times > 0) {
        swipe_to_the_left_of_operlist(swipe_times + 1);
    }
    return true;
}
