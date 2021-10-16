#include "InfrastConfiger.h"

#include <json.h>

bool asst::InfrastConfiger::parse(const json::value& json)
{
    for (const json::value& facility : json.at("facility").as_array()) {
        std::string facility_name = facility.as_string();
        std::vector<InfrastSkill> facility_skills;
        for (const json::value& skill_json : json.at(facility_name).at("skills").as_array()) {
            InfrastSkill skill;
            skill.templ_name = skill_json.at("template").as_string();
            m_templ_required.emplace(skill.templ_name);

            std::string id = skill_json.get("id", skill.templ_name);
            skill.id = id;
            if (skill_json.exist("name")) {
                for (const json::value& skill_names : skill_json.at("name").as_array()) {
                    skill.names.emplace_back(skill_names.as_string());
                }
            }
            skill.intro = skill_json.get("intro", std::string());

            facility_skills.emplace_back(std::move(skill));
        }
        m_skills.emplace(std::move(facility_name), std::move(facility_skills));
    }
    return false;
}