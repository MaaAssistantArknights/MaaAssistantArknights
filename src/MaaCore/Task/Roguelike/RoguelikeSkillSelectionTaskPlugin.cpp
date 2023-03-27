#include "RoguelikeSkillSelectionTaskPlugin.h"

#include "Config/Roguelike/RoguelikeRecruitConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Vision/Roguelike/RoguelikeSkillSelectionImageAnalyzer.h"

#include "Utils/Logger.hpp"

bool asst::RoguelikeSkillSelectionTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    auto roguelike_name_opt = status()->get_properties(Status::RoguelikeTheme);
    if (!roguelike_name_opt) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = std::move(roguelike_name_opt.value()) + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@StartAction") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeSkillSelectionTaskPlugin::_run()
{
    LogTraceFunction;

    auto image = ctrler()->get_image();
    RoguelikeSkillSelectionImageAnalyzer analyzer(image);

    if (!analyzer.analyze()) {
        return false;
    }

    int delay = Task.get("RoguelikeSkillSelectionMove1")->post_delay;
    bool has_rookie = false;
    for (const auto& [name, skill_vec] : analyzer.get_result()) {
        const auto& oper_info =
            RoguelikeRecruit.get_oper_info(status()->get_properties(Status::RoguelikeTheme).value(), name);
        if (oper_info.name.empty()) {
            Log.warn("Unknown oper", name);
            continue;
        }

        if (oper_info.alternate_skill > 0) {
            Log.info(__FUNCTION__, name, " select alternate skill:", oper_info.alternate_skill);
            ctrler()->click(skill_vec.at(oper_info.alternate_skill - 1));
            sleep(delay);
        }
        if (oper_info.skill > 0) {
            Log.info(__FUNCTION__, name, " select main skill:", oper_info.skill);
            ctrler()->click(skill_vec.at(oper_info.skill - 1));
            sleep(delay);
        }
        constexpr int RookieStd = 200;
        if (oper_info.promote_priority < RookieStd) {
            has_rookie = true;
        }
        status()->set_number(Status::RoguelikeSkillUsagePrefix + name, static_cast<int>(oper_info.skill_usage));
    }

    if (!status()->get_str(Status::RoguelikeCharOverview)) {
        json::value overview;
        for (const auto& [name, skill_vec] : analyzer.get_result()) {
            overview[name] = json::object {
                { "elite", 1 }, { "level", 80 },
                // 不知道是啥等级随便填一个
            };
        }
        status()->set_str(Status::RoguelikeCharOverview, overview.to_string());
    }

    if (analyzer.get_team_full() && !has_rookie) {
        Log.info("Team full and no rookie");
        status()->set_number(Status::RoguelikeTeamFullWithoutRookie, 1);
    }
    else {
        Log.info("Team not full or has rookie");
        status()->set_number(Status::RoguelikeTeamFullWithoutRookie, 0);
    }
    return true;
}
