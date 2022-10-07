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

    if (details.at("details").at("task").as_string() == "Roguelike1StartAction") {
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

    bool has_rookie = false;
    for (const auto& [name, skill_vec] : analyzer.get_result()) {
        if (name.empty()) {
            continue;
        }
        const auto& oper_info = RoguelikeRecruit.get_oper_info(name);

        if (oper_info.alternate_skill > 0) {
            Log.info(__FUNCTION__, name, " select alternate skill:", oper_info.alternate_skill);
            m_ctrler->click(skill_vec.at(oper_info.alternate_skill - 1));
        }
        if (oper_info.skill > 0) {
            Log.info(__FUNCTION__, name, " select main skill:", oper_info.skill);
            m_ctrler->click(skill_vec.at(oper_info.skill - 1));
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
