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
        Log.info(__FUNCTION__, name, " skill size:", skill_vec.size());
        if (name.empty()) {
            continue;
        }
        const auto& oper_info = rg_src.get_oper_info(name);
        int index = 0;
        auto usage = BattleSkillUsage::Possibly;
        if (skill_vec.size() >= oper_info.skill) {
            index = oper_info.skill - 1;
            usage = oper_info.skill_usage;
        }
        else if (skill_vec.size() >= oper_info.alternate_skill) {
            index = oper_info.alternate_skill - 1;
            usage = oper_info.alternate_skill_usage;
        }

        if (index >= 0) {
            Log.info(__FUNCTION__, name, " select skill:", index + 1);
            m_ctrler->click(skill_vec.at(index));
        }
        m_status->set_number("RoguelikeSkillUsage-" + name, static_cast<int>(usage));
    }

    return true;
}
