#include "InfrastConfiger.h"

#include <json.h>

bool asst::InfrastConfiger::parse(const json::value& json)
{
    for (const json::value& facility : json.at("facility").as_array()) {
        for (const json::value& skill_json : json.at(facility.as_string()).as_array()) {
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

            m_skills.emplace(std::move(id), std::move(skill));
        }
    }
    return false;
}