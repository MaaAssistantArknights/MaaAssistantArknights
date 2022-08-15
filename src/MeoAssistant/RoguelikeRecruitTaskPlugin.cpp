#include "RoguelikeRecruitTaskPlugin.h"

#include "RoguelikeRecruitImageAnalyzer.h"
#include "Controller.h"
#include "TaskData.h"
#include "Resource.h"
#include "Logger.hpp"
#include "RuntimeStatus.h"
#include "ProcessTask.h"

bool asst::RoguelikeRecruitTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted
        || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "Roguelike1ChooseOper") {
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

    const auto& recruit_cfg = Resrc.roguelike_recruit();

    // 编队信息 (已有角色)
    std::string str_chars_info = m_status->get_str(RuntimeStatus::RoguelikeCharOverview).value_or(json::value().to_string());
    json::value json_chars_info = json::parse(str_chars_info).value_or(json::value());
    const auto& chars_map = json_chars_info.as_object();

    // 候选干员
    std::vector<RoguelikeRecruitInfo> recruit_list;

    constexpr int SwipeTimes = 5;
    int i = 0;

    // 翻页找出所有候选干员
    for (; i != SwipeTimes; ++i) {
        auto image = m_ctrler->get_image();
        RoguelikeRecruitImageAnalyzer analyzer(image);
        if (!analyzer.analyze()) {
            // 本页没有检测到符合条件的干员，向右滑动
            slowly_swipe(ProcessTaskAction::SlowlySwipeToTheRight);
            sleep(Task.get("Roguelike1Custom-HijackSquad")->rear_delay);
            continue;
        }

        const auto& oper_list = analyzer.get_result();
        bool stop_swipe = false;

        for (const auto& oper_info : oper_list) {
            // 查询招募配置
            auto& recruit_info = recruit_cfg.get_oper_info(oper_info.name);
            if (recruit_info.name.empty()) {
                continue;
            }

            int priority = 0;

            // 查询编队是否已持有该干员
            const auto& char_opt = chars_map.find(oper_info.name);

            if (char_opt.has_value()) {
                // 干员已在编队中
                int elite = char_opt.value().get("elite", 0);
                if (elite == 2) {
                    // 干员已精二，忽略
                    continue;
                }

                if (recruit_info.is_alternate) {
                    // 预备干员可以重复招募
                    priority = recruit_info.recruit_priority;
                }
                else {
                    // 干员待晋升
                    priority = recruit_info.promote_priority;
                }
            }
            else {
                // 干员待招募
                if (oper_info.elite == 2) {
                    // TODO: 招募时已精二 (随机提升或对应分队) => promoted_priority
                    priority = recruit_info.recruit_priority;
                }
                else {
                    // 招募时未精二
                    priority = recruit_info.recruit_priority;
                }
            }

            // 已经划到后备干员，且已有候选干员，假定后面的干员优先级不会更高，不再划动
            if (recruit_info.is_alternate && !recruit_list.empty()) {
                stop_swipe = true;
            }

            // 添加到候选名单
            auto existing_it = ranges::find_if(recruit_list, [&](const RoguelikeRecruitInfo& pri) -> bool {
                return pri.name == recruit_info.name;
            });
            if (existing_it == recruit_list.cend()) {
                RoguelikeRecruitInfo info;
                info.name = recruit_info.name;
                info.priority = priority;
                info.is_alternate = recruit_info.is_alternate;
                info.page_index = i;
                recruit_list.emplace_back(info);
            }
        }

        if (stop_swipe) {
            break;
        }

        // 向右滑动
        slowly_swipe(ProcessTaskAction::SlowlySwipeToTheRight);
        sleep(Task.get("Roguelike1Custom-HijackSquad")->rear_delay);
    }

    // 没有候选干员，进入后备逻辑
    if (recruit_list.empty()) {
        Log.trace(__FUNCTION__, "| No oper in recruit list.");

        // 如果划动过，先划回最左边
        if (i != 0) {
            swipe_to_the_left_of_operlist(i / 2 + 1);
        }

        auto image = m_ctrler->get_image();
        RoguelikeRecruitImageAnalyzer analyzer(image);
        if (!analyzer.analyze()) {
            Log.error(__FUNCTION__, "| Random recruitment analyse failed");
            return false;
        }

        const auto& oper_list = analyzer.get_result();

        // 可能是晋升界面，随便选个精二的
        for (const auto& info : oper_list) {
            if (!info.required) {
                continue;
            }
            if (info.elite != 2) {
                continue;
            }
            Log.trace(__FUNCTION__, "| Choose random elite 2:", info.name, info.elite, info.level);
            recruit_oper(info);
            return true;
        }

        // 仍然没选，随便选个精一 50 以上的
        for (const auto& info : oper_list) {
            if (!info.required) {
                continue;
            }
            if (info.elite == 0 ||
                (info.elite == 1 && info.level < 50)) {
                continue;
            }
            Log.trace(__FUNCTION__, "| Choose random elite 1:", info.name, info.elite, info.level);
            recruit_oper(info);
            return true;
        }

        Log.trace(__FUNCTION__, "| Did not choose oper");
        return false;
    }

    // 选择优先级最高的干员
    auto selected_oper = ranges::max_element(recruit_list, [&](const asst::RoguelikeRecruitInfo& lhs, const asst::RoguelikeRecruitInfo& rhs) -> int {
        return rhs.priority > lhs.priority;
    });
    if (selected_oper == recruit_list.cend()) {
        Log.trace(__FUNCTION__, "| No opers in recruit list.");
        return false;
    }

    auto& char_name = selected_oper->name;
    Log.trace(__FUNCTION__, "| Top priority oper:", char_name, selected_oper->priority, "page", selected_oper->page_index, "/", i);

    // 滑动方向
    // 页码大于一半: 从右往左划动
    // 页码小于一半: 划回最左边再往右
    bool is_rtl = false;

    // 如果划动过，判断目标角色离哪个方向更近
    if (i != 0) {
        is_rtl = (selected_oper->page_index * 2) >= i;
        if (!is_rtl) {
            // 从左往右需要先划回最左边
            swipe_to_the_left_of_operlist(i / 2 + 1);
        }
    }

    return check_char(char_name, is_rtl);
}

bool asst::RoguelikeRecruitTaskPlugin::check_char(const std::string& char_name, bool is_rtl)
{
    LogTraceFunction;

    constexpr int SwipeTimes = 10;
    int i = 0;
    for (; i != SwipeTimes; ++i) {
        auto image = m_ctrler->get_image();
        RoguelikeRecruitImageAnalyzer analyzer(image);

        // 只处理识别成功的情况，失败(无任何结果)时继续滑动
        if (analyzer.analyze()) {
            const auto& chars = analyzer.get_result();
            auto it = ranges::find_if(chars,
                [&](const BattleRecruitOperInfo& oper) -> bool {
                    return oper.name == char_name;
                });

            std::string oper_names = "";
            for (const auto& oper : chars) {
                if (!oper_names.empty()) {
                    oper_names += ", ";
                }
                oper_names += oper.name;
            }
            Log.info(__FUNCTION__, "| Oper list:", oper_names);

            if (it != chars.cend()) {
                select_oper(*it);
                return true;
            }
        }

        // 没识别到目标干员，可能不在这一页，继续划动
        if (is_rtl) {
            slowly_swipe(ProcessTaskAction::SlowlySwipeToTheLeft);
        }
        else {
            slowly_swipe(ProcessTaskAction::SlowlySwipeToTheRight);
        }
        sleep(Task.get("Roguelike1Custom-HijackSquad")->rear_delay);
    }
    Log.info(__FUNCTION__, "| Cannot find oper `" + char_name + "`");
    swipe_to_the_left_of_operlist();
    return false;
}

bool asst::RoguelikeRecruitTaskPlugin::check_core_char()
{
    LogTraceFunction;

    auto core_opt = m_status->get_str("RoguelikeCoreChar");
    if (!core_opt || core_opt->empty()) {
        return false;
    }
    m_status->set_str("RoguelikeCoreChar", "");
    return check_char(core_opt.value());
}

void asst::RoguelikeRecruitTaskPlugin::select_oper(const BattleRecruitOperInfo& oper)
{
    Log.info(__FUNCTION__, "| Choose oper:", oper.name, "( elite", oper.elite, "level", oper.level, ")");

    m_ctrler->click(oper.rect);

    m_status->set_number(RuntimeStatus::RoguelikeCharElitePrefix + oper.name, oper.elite);
    m_status->set_number(RuntimeStatus::RoguelikeCharLevelPrefix + oper.name, oper.level);

    std::string overview_str = m_status->get_str(RuntimeStatus::RoguelikeCharOverview).value_or(json::value().to_string());
    json::value overview = json::parse(overview_str).value_or(json::value());
    overview[oper.name] = json::object{
        { "elite", oper.elite },
        { "level", oper.level },
    };
    m_status->set_str(RuntimeStatus::RoguelikeCharOverview, overview.to_string());
}

void asst::RoguelikeRecruitTaskPlugin::swipe_to_the_left_of_operlist(int loop_times)
{
    LogTraceFunction;
    static Rect begin_rect = Task.get("RoguelikeRecruitSwipeToTheLeftBegin")->specific_rect;    // 1080, 200, 100, 300
    static Rect end_rect = Task.get("RoguelikeRecruitSwipeToTheLeftEnd")->specific_rect;        // 400, 200, 100, 300
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
    static Rect right_rect = Task.get("RoguelikeRecruitSlowlySwipeRightRect")->specific_rect;   // 780, 200, 100, 300
    static Rect left_rect = Task.get("RoguelikeRecruitSlowlySwipeLeftRect")->specific_rect;     // 360, 200, 100, 300
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
