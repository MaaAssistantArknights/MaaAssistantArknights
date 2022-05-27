#include "RoguelikeSkillSelectionTaskPlugin.h"

#include "RoguelikeSkillSelectionImageAnalyzer.h"
#include "Controller.h"
#include "TaskData.h"
#include "Resource.h"

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
    auto image = m_ctrler->get_image();
    RoguelikeSkillSelectionImageAnalyzer analyzer(image);

    if (!analyzer.analyze()) {
        return false;
    }

    const auto& rg_src = Resrc.roguelike_recruit();
    for (const auto& [name, skill_vec] : analyzer.get_result()) {
        const auto& oper_info = rg_src.get_oper_info(name);
        int index = 0;
        if (skill_vec.size() >= oper_info.skill) {
            index = oper_info.skill - 1;
        }
        else if (skill_vec.size() >= oper_info.alternate_skill) {
            index = oper_info.alternate_skill - 1;
        }

        if (index) {
            m_ctrler->click(skill_vec.at(index));
        }
    }

    return true;
}
