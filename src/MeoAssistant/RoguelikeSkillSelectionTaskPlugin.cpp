#include "RoguelikeSkillSelectionTaskPlugin.h"

#include "RoguelikeSkillSelectionImageAnalyzer.h"
#include "Controller.h"
#include "TaskData.h"

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

void asst::RoguelikeSkillSelectionTaskPlugin::set_skill_map(SkillMap skill_mapping)
{
    m_skill_mapping = std::move(skill_mapping);
}

bool asst::RoguelikeSkillSelectionTaskPlugin::_run()
{
    //#ifdef ASST_DEBUG
    //    m_skill_mapping.clear();
    //    m_skill_mapping.reserve(4);
    //    m_skill_mapping.emplace("山", 2);
    //    m_skill_mapping.emplace("芙蓉", 1);
    //    m_skill_mapping.emplace("梓兰", 1);
    //    m_skill_mapping.emplace("阿米娅", 3);
    //    m_skill_mapping.emplace("Unknown", 3);
    //#endif
    auto image = m_ctrler->get_image();
    RoguelikeSkillSelectionImageAnalyzer analyzer(image);

    if (!analyzer.analyze()) {
        return false;
    }

    for (const auto& [name, skill_vec] : analyzer.get_result()) {
        int cor_req_skill = 3;
        for (const auto& [req_name, req_skill] : m_skill_mapping) {
            if (name.find(req_name) != std::string::npos) {
                cor_req_skill = req_skill;
                break;
            }
        }
        size_t index = 0;
        if (skill_vec.size() < cor_req_skill) {
            index = skill_vec.size() - 1;
            Log.info("skill", cor_req_skill, "not exists, use skill", index);
        }
        else {
            index = cor_req_skill - 1;
        }
        Rect target = skill_vec.at(index);
        m_ctrler->click(target);
    }

    return true;
}
