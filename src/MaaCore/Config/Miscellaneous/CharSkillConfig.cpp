#include "CharSkillConfig.h"

#include "Utils/Logger.hpp"
#include <meojson/json.hpp>

bool asst::CharSkillConfig::parse(const json::value& json)
{
    LogTraceFunction;


    for (auto& [char_name, char_opt] : json.as_object()) {

        for (auto& [lang, skills_opt] : char_opt.as_object()) {
            auto& skills = skills_opt.as_array();

            Language lang_enum;
            if (lang == "zh_CN") {
                lang_enum = Language::ZH_CN;
            } else {
                LogError << "Unknown language: " << lang;
                return false;
            }

            for (auto& skill_name : skills) {
                m_skill_name[lang_enum][char_name].push_back(skill_name.as_string());
            }

        }
    }
    return true;
}

std::string asst::CharSkillConfig::get_skill_name_by_pos(const std::string& char_name, int pos, Language language)
{
    return m_skill_name[language][char_name][pos - 1];
}

