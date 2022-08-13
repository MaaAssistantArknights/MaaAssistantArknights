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

    constexpr int SwipeTimes = 5;
    int i = 0;
    for (; i != SwipeTimes; ++i) {
        auto image = m_ctrler->get_image();
        RoguelikeRecruitImageAnalyzer analyzer(image);
        if (!analyzer.analyze()) {
            return false;
        }

        const auto& oper_list = analyzer.get_result();
        for (const auto& info : oper_list) {
            // 先看看有没有精二的
            if (info.elite != 2) {
                continue;
            }
            recruit_oper(info);
            break;
        }

        if (!recruited) {
            for (const auto& info : oper_list) {
                if (!info.required) {
                    continue;
                }
                // 拿个精一 50 以上的
                if (info.elite == 0 ||
                    (info.elite == 1 && info.level < 50)) {
                    continue;
                }
                recruit_oper(info);
                break;
            }
        }

        if (!recruited) {
            Log.info("All are lower");
            // 随便招个精一的
            for (const auto& info : oper_list) {
                if (!info.required) {
                    continue;
                }
                if (info.elite == 0) {
                    continue;
                }
                recruit_oper(info);
                break;
            }
        }

        // 这玩意选了也没啥用，不如省点理智
        //if (!recruited) {
        //    // 随便招个
        //    Log.info("All are very lower");
        //    auto info = oper_list.front();
        //    recruit_oper(info);
        //}

        // 没有合适的干员，尝试向右翻页
        if (!recruited) {
            ProcessTask(*this, { "SlowlySwipeToTheRight" }).run();
            sleep(Task.get("Roguelike1Custom-HijackSquad")->rear_delay);
            continue;
        }

        // 已经招募到合适的干员
        return true;
    }

    Log.info(__FUNCTION__, "did not choose oper.");
    swipe_to_the_left_of_operlist(i / 2 + 1);
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
    constexpr int SwipeTimes = 10;
    int i = 0;
    for (; i != SwipeTimes; ++i) {
        auto image = m_ctrler->get_image();
        RoguelikeRecruitImageAnalyzer analyzer(image);
        if (!analyzer.analyze()) {
            Log.info(__FUNCTION__, "| Unable to analyze image.");
            break;
        }
        const auto& chars = analyzer.get_result();
        auto it = ranges::find_if(chars,
            [&](const BattleRecruitOperInfo& oper) -> bool {
                return oper.name == core_opt.value();
            });

        std::string oper_names = "";
        for (const auto& oper : chars) {
            if (!oper_names.empty()) {
                oper_names += ", ";
            }
            oper_names += oper.name;
        }
        Log.info(__FUNCTION__, "| Oper list:", oper_names);
        if (it == chars.cend()) {
            slowly_swipe(ProcessTaskAction::SlowlySwipeToTheRight);
            sleep(Task.get("Roguelike1Custom-HijackSquad")->rear_delay);
            continue;
        }
        select_oper(*it);
        return true;
    }
    Log.info(__FUNCTION__, "| Cannot find oper `" + core_opt.value() + "`");
    swipe_to_the_left_of_operlist();
    return false;
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
    static Rect begin_rect = Task.get("RoguelikeRecruitSwipeToTheLeftBegin")->specific_rect;
    static Rect end_rect = Task.get("RoguelikeRecruitSwipeToTheLeftEnd")->specific_rect;
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
    static Rect right_rect = Task.get("RoguelikeRecruitSlowlySwipeRightRect")->specific_rect;
    static Rect left_rect = Task.get("RoguelikeRecruitSlowlySwipeLeftRect")->specific_rect;
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
