#include "RoguelikeSkillSelectionTaskPlugin.h"

#include "Controller.h"
#include "ImageAnalyzer/RoguelikeSkillSelectionImageAnalyzer.h"
#include "Resource/RoguelikeRecruitConfiger.h"
#include "RuntimeStatus.h"
#include "TaskData.h"

#include "Utils/Logger.hpp"

bool asst::RoguelikeSkillSelectionTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    auto roguelike_name_opt = m_status->get_properties(RuntimeStatus::RoguelikeTheme);
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

    auto image = m_ctrler->get_image();
    RoguelikeSkillSelectionImageAnalyzer analyzer(image);

    if (!analyzer.analyze()) {
        return false;
    }

    int delay = Task.get("RoguelikeSkillSelectionMove1")->rear_delay;
    bool has_rookie = false;
    for (const auto& [name, skill_vec] : analyzer.get_result()) {
        if (name.empty()) {
            continue;
        }
        const auto& oper_info = RoguelikeRecruit.get_oper_info(name);

        if (oper_info.alternate_skill > 0) {
            Log.info(__FUNCTION__, name, " select alternate skill:", oper_info.alternate_skill);
            m_ctrler->click(skill_vec.at(oper_info.alternate_skill - 1));
            sleep(delay);
        }
        if (oper_info.skill > 0) {
            Log.info(__FUNCTION__, name, " select main skill:", oper_info.skill);
            m_ctrler->click(skill_vec.at(oper_info.skill - 1));
            sleep(delay);
        }
        constexpr int RookieStd = 200;
        if (oper_info.promote_priority < RookieStd) {
            has_rookie = true;
        }
        m_status->set_number(RuntimeStatus::RoguelikeSkillUsagePrefix + name, static_cast<int>(oper_info.skill_usage));
    }

    if (analyzer.get_team_full() && !has_rookie) {
        Log.info("Team full and no rookie");
        m_status->set_number(RuntimeStatus::RoguelikeTeamFullWithoutRookie, 1);
    }
    else {
        Log.info("Team not full or has rookie");
        m_status->set_number(RuntimeStatus::RoguelikeTeamFullWithoutRookie, 0);
    }
    return true;
}