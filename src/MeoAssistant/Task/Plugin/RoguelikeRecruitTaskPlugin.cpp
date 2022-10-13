#include "RoguelikeRecruitTaskPlugin.h"

#include "../Sub/ProcessTask.h"
#include "Controller.h"
#include "ImageAnalyzer/RoguelikeRecruitImageAnalyzer.h"
#include "Resource/RoguelikeRecruitConfiger.h"
#include "RuntimeStatus.h"
#include "TaskData.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeRecruitTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    auto roguelike_name_opt = m_status->get_properties("roguelike_name");
    if (!roguelike_name_opt) {
        return false;
    }
    const auto& roguelike_name = roguelike_name_opt.value() + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@ChooseOper") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeRecruitTaskPlugin::_run()
{
    LogTraceFunction;

    if (check_core_char()) {
        return true;
    }

    bool recruited = false;

    auto recruit_oper = [&](const BattleRecruitOperInfo& info) {
        select_oper(info);
        recruited = true;
    };

    bool team_full_without_rookie = m_status->get_number(RuntimeStatus::RoguelikeTeamFullWithoutRookie).value_or(0);
    Log.info("team_full_without_rookie", team_full_without_rookie);

    // 编队信息 (已有角色)
    std::string str_chars_info =
        m_status->get_str(RuntimeStatus::RoguelikeCharOverview).value_or(json::value().to_string());
    json::value json_chars_info = json::parse(str_chars_info).value_or(json::value());
    const auto& chars_map = json_chars_info.as_object();

    // 候选干员
    std::vector<RoguelikeRecruitInfo> recruit_list;

    // 干员名字的识别位置
    std::unordered_map<std::string, Rect> last_oper_rects;

    int SwipeTimes = Task.get("RoguelikeRecruitSwipeMaxTime")->max_times;
    int i = 0;
    std::unordered_set<std::string> pre_oper_names;
    // 翻页找出所有候选干员
    for (; i != SwipeTimes; ++i) {
        if (need_exit()) {
            return false;
        }
        auto image = m_ctrler->get_image();
        RoguelikeRecruitImageAnalyzer analyzer(image);
        if (!analyzer.analyze()) {
            Log.trace(__FUNCTION__, "| Page", i, "recruit list analyse failed");
            // 还没滑动就识别失败，通常是招募界面为空，视为招募成功并退出
            if (i == 0) {
                return true;
            }
            // 已经滑动过，识别失败可能是干员不可选
            break;
        }

        std::unordered_set<std::string> oper_names;
        const auto& oper_list = analyzer.get_result();
        bool stop_swipe = false;

        for (const auto& oper_info : oper_list) {
            oper_names.emplace(oper_info.name);

            // 查询上次识别位置
            const auto& rect_it = last_oper_rects.find(oper_info.name);
            if (rect_it == last_oper_rects.cend()) {
                last_oper_rects.emplace(oper_info.name, oper_info.rect);
            }
            else {
                const auto x_offset = abs(rect_it->second.x - oper_info.rect.x);
                const auto y_offset = abs(rect_it->second.y - oper_info.rect.y);

                // 偏移低于阈值，代表划动没有效果，已经到达屏幕最右侧
                if (x_offset < 20 && y_offset < 5) {
                    stop_swipe = true;
                    Log.trace(__FUNCTION__, "| Page", i, "oper", oper_info.name,
                              "last rect:", rect_it->second.to_string(), "current rect:", oper_info.rect.to_string(),
                              " - stop swiping");
                    break;
                }

                rect_it->second = oper_info.rect;
            }

            // 查询招募配置
            auto& recruit_info = RoguelikeRecruit.get_oper_info(oper_info.name);
            if (recruit_info.name.empty()) {
                continue;
            }

            int priority = 0;

            // 查询编队是否已持有该干员
            const auto& char_opt = chars_map.find(oper_info.name);

            if (char_opt.has_value()) {
                // 干员已在编队中，又出现在招募列表，只有待晋升和预备干员两种情况
                if (recruit_info.is_alternate) {
                    // 预备干员可以重复招募
                    priority = team_full_without_rookie ? recruit_info.recruit_priority_when_team_full
                                                        : recruit_info.recruit_priority;
                }
                else {
                    // 干员待晋升
                    priority = team_full_without_rookie ? recruit_info.promote_priority_when_team_full
                                                        : recruit_info.promote_priority;
                }
            }
            else {
                // 干员待招募，检查练度
                if (oper_info.elite == 2) {
                    // TODO: 招募时已精二 (随机提升或对应分队) => promoted_priority
                    priority = team_full_without_rookie ? recruit_info.recruit_priority_when_team_full
                                                        : recruit_info.recruit_priority;
                }
                else if (oper_info.elite == 1 && oper_info.level >= 50) {
                    // 精一50级以上
                    priority = team_full_without_rookie ? recruit_info.recruit_priority_when_team_full
                                                        : recruit_info.recruit_priority;
                }
                else {
                    // 精一50级以下，默认不招募
                    Log.trace(__FUNCTION__, "| Ignored low level oper:", oper_info.name, oper_info.elite,
                              oper_info.level);
                }
            }

            // 优先级为0，可能练度不够被忽略
            if (priority <= 0) {
                continue;
            }

            // 添加到候选名单
            auto existing_it = ranges::find_if(
                recruit_list, [&](const RoguelikeRecruitInfo& pri) -> bool { return pri.name == recruit_info.name; });
            if (existing_it == recruit_list.cend()) {
                RoguelikeRecruitInfo info;
                info.name = recruit_info.name;
                info.priority = priority;
                info.is_alternate = recruit_info.is_alternate;
                info.page_index = i;
                recruit_list.emplace_back(info);
            }
        }

        Log.info(__FUNCTION__, "| Page", i, "oper list:", oper_names);

        if (stop_swipe) {
            break;
        }

        // 每列4个干员，未滑动时可以显示2列，滑动后至少可以显示1列
        const size_t oper_count = oper_list.size();
        if ((i == 0 && oper_count < 8) || (oper_count != 4 && oper_count != 8)) {
            Log.trace(__FUNCTION__, "| Page", i, "oper count:", oper_count, "- stop swiping");
            break;
        }
        if (pre_oper_names == oper_names) {
            Log.trace(__FUNCTION__, "| Oper list not changed, stop swiping");
            break;
        }
        pre_oper_names = std::move(oper_names);

        // 向右滑动
        Log.trace(__FUNCTION__, "| Page", i, "oper count:", oper_count, "- continue swiping");
        slowly_swipe(ProcessTaskAction::SlowlySwipeToTheRight);
        sleep(Task.get("RoguelikeCustom-HijackSquad")->rear_delay);
    }

    // 没有候选干员，进入后备逻辑
    if (recruit_list.empty()) {
        Log.trace(__FUNCTION__, "| No oper in recruit list.");

        // 如果划动过，先划回最左边
        if (i != 0) {
            swipe_to_the_left_of_operlist(i / 3 + 1);
        }

        auto image = m_ctrler->get_image();
        RoguelikeRecruitImageAnalyzer analyzer(image);
        if (!analyzer.analyze()) {
            Log.error(__FUNCTION__, "| Random recruitment analyse failed");
            return false;
        }

        const auto& oper_list = analyzer.get_result();

        // 可能有临时招募的精二的，不要白不要
        for (const auto& info : oper_list) {
            if (info.elite != 2) {
                continue;
            }
            Log.trace(__FUNCTION__, "| Choose random elite 2:", info.name, info.elite, info.level);
            recruit_oper(info);
            return true;
        }

        //// 仍然没选，随便选个精一 50 以上的
        // for (const auto& info : oper_list) {
        //     if (info.elite == 0 || (info.elite == 1 && info.level < 50)) {
        //         continue;
        //     }
        //     Log.trace(__FUNCTION__, "| Choose random elite 1:", info.name, info.elite, info.level);
        //     recruit_oper(info);
        //     return true;
        // }

        Log.trace(__FUNCTION__, "| Did not choose oper");
        return true;
    }

    // 选择优先级最高的干员
    auto selected_oper = ranges::max_element(recruit_list, std::less {}, std::mem_fn(&RoguelikeRecruitInfo::priority));
    if (selected_oper == recruit_list.cend()) {
        Log.trace(__FUNCTION__, "| No opers in recruit list.");
        return false;
    }

    auto& char_name = selected_oper->name;
    Log.trace(__FUNCTION__, "| Top priority oper:", char_name, selected_oper->priority, "page",
              selected_oper->page_index, "/", i);

    // 滑动方向
    // 页码大于一半: 从右往左划动
    // 页码小于一半: 划回最左边再往右
    bool is_rtl = false;

    // 如果划动过，判断目标角色离哪个方向更近
    if (i != 0) {
        is_rtl = (selected_oper->page_index * 2) >= i;
        if (!is_rtl) {
            // 从左往右需要先划回最左边
            swipe_to_the_left_of_operlist(i / 3 + 1);
        }
    }

    return check_char(char_name, is_rtl);
}

bool asst::RoguelikeRecruitTaskPlugin::check_char(const std::string& char_name, bool is_rtl)
{
    LogTraceFunction;

    constexpr int SwipeTimes = 5;
    std::unordered_set<std::string> pre_oper_names;
    bool has_been_same = false;
    int i = 0;
    for (; i != SwipeTimes; ++i) {
        if (need_exit()) {
            return false;
        }
        auto image = m_ctrler->get_image();
        RoguelikeRecruitImageAnalyzer analyzer(image);

        // 只处理识别成功的情况，失败(无任何结果)时继续滑动
        if (analyzer.analyze()) {
            const auto& chars = analyzer.get_result();
            auto it = ranges::find_if(
                chars, [&](const BattleRecruitOperInfo& oper) -> bool { return oper.name == char_name; });

            std::unordered_set<std::string> oper_names;
            ranges::transform(chars, std::inserter(oper_names, oper_names.end()),
                              std::mem_fn(&BattleRecruitOperInfo::name));
            Log.info(__FUNCTION__, "| Oper list:", oper_names);

            if (it != chars.cend()) {
                select_oper(*it);
                return true;
            }
            if (pre_oper_names == oper_names) {
                if (has_been_same) {
                    Log.trace(__FUNCTION__, "| Oper list not changed for three times, stop swiping");
                    break;
                }
                else {
                    has_been_same = true;
                }
            }
            else {
                has_been_same = false;
                pre_oper_names = std::move(oper_names);
            }
        }

        // 没识别到目标干员，可能不在这一页，继续划动
        if (is_rtl) {
            slowly_swipe(ProcessTaskAction::SlowlySwipeToTheLeft);
        }
        else {
            slowly_swipe(ProcessTaskAction::SlowlySwipeToTheRight);
        }
        sleep(Task.get("RoguelikeCustom-HijackSquad")->rear_delay);
    }
    Log.info(__FUNCTION__, "| Cannot find oper `" + char_name + "`");
    swipe_to_the_left_of_operlist(i / 3 + 1);
    return false;
}

bool asst::RoguelikeRecruitTaskPlugin::check_core_char()
{
    LogTraceFunction;

    auto core_opt = m_status->get_str(RuntimeStatus::RoguelikeCoreChar);
    if (!core_opt || core_opt->empty()) {
        return false;
    }
    m_status->set_str(RuntimeStatus::RoguelikeCoreChar, "");
    return check_char(core_opt.value());
}

void asst::RoguelikeRecruitTaskPlugin::select_oper(const BattleRecruitOperInfo& oper)
{
    Log.info(__FUNCTION__, "| Choose oper:", oper.name, "( elite", oper.elite, "level", oper.level, ")");

    m_ctrler->click(oper.rect);

    m_status->set_number(RuntimeStatus::RoguelikeCharElitePrefix + oper.name, oper.elite);
    m_status->set_number(RuntimeStatus::RoguelikeCharLevelPrefix + oper.name, oper.level);

    std::string overview_str =
        m_status->get_str(RuntimeStatus::RoguelikeCharOverview).value_or(json::value().to_string());
    json::value overview = json::parse(overview_str).value_or(json::value());
    overview[oper.name] = json::object {
        { "elite", oper.elite },
        { "level", oper.level },
    };
    m_status->set_str(RuntimeStatus::RoguelikeCharOverview, overview.to_string());
}

void asst::RoguelikeRecruitTaskPlugin::swipe_to_the_left_of_operlist(int loop_times)
{
    LogTraceFunction;
    static Rect begin_rect = Task.get("RoguelikeRecruitSwipeToTheLeftBegin")->specific_rect; // 1080, 200, 100, 300
    static Rect end_rect = Task.get("RoguelikeRecruitSwipeToTheLeftEnd")->specific_rect;     // 400, 200, 100, 300
    static int duration = Task.get("RoguelikeRecruitSwipeToTheLeftBegin")->pre_delay;
    static int extra_delay = Task.get("RoguelikeRecruitSwipeToTheLeftBegin")->rear_delay;
    static int cfg_loop_times = Task.get("RoguelikeRecruitSwipeToTheLeftBegin")->max_times;

    for (int i = 0; i != cfg_loop_times * loop_times; ++i) {
        if (need_exit()) {
            return;
        }
        m_ctrler->swipe(end_rect, begin_rect, duration, true, 0, false);
    }
    sleep(extra_delay);
}

void asst::RoguelikeRecruitTaskPlugin::slowly_swipe(ProcessTaskAction action)
{
    LogTraceFunction;
    static Rect right_rect = Task.get("RoguelikeRecruitSlowlySwipeRightRect")->specific_rect; // 980, 200, 100, 300
    static Rect left_rect = Task.get("RoguelikeRecruitSlowlySwipeLeftRect")->specific_rect;   // 560, 200, 100, 300
    static int duration = Task.get("RoguelikeRecruitSlowlySwipeRightRect")->pre_delay;
    static int extra_delay = Task.get("RoguelikeRecruitSlowlySwipeRightRect")->rear_delay;

    switch (action) {
    case asst::ProcessTaskAction::SlowlySwipeToTheLeft:
        m_ctrler->swipe(left_rect, right_rect, duration, true, extra_delay, true);
        break;
    case asst::ProcessTaskAction::SlowlySwipeToTheRight:
        m_ctrler->swipe(right_rect, left_rect, duration, true, extra_delay, true);
        break;
    default: // 走不到这里
        break;
    }
}
