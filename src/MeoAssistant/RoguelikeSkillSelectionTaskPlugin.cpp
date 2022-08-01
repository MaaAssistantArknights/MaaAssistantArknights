#include "RoguelikeSkillSelectionTaskPlugin.h"

#include "RoguelikeSkillSelectionImageAnalyzer.h"
#include "Controller.h"
#include "TaskData.h"
#include "Resource.h"
#include "RuntimeStatus.h"

#include "Logger.hpp"

bool asst::RoguelikeSkillSelectionTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart
        || details.get("subtask", std::string()) != "ProcessTask") {
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

    const auto& rg_src = Resrc.roguelike_recruit();
    for (const auto& [name, skill_vec] : analyzer.get_result()) {
        if (name.empty()) {
            continue;
        }
        const auto& oper_info = rg_src.get_oper_info(name);

        if (oper_info.alternate_skill > 0) {
            Log.info(__FUNCTION__, name, " select alternate skill:", oper_info.alternate_skill);
            m_ctrler->click(skill_vec.at(oper_info.alternate_skill - 1));
        }
        if (oper_info.skill > 0) {
            Log.info(__FUNCTION__, name, " select main skill:", oper_info.skill);
            m_ctrler->click(skill_vec.at(oper_info.skill - 1));
        }
        m_status->set_number("RoguelikeSkillUsage-" + name, static_cast<int>(oper_info.skill_usage));
    }

    return true;
}
