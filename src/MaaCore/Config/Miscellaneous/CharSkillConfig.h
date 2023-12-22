#pragma once
#include "Config/AbstractConfig.h"
#include "Common/AsstTypes.h"

namespace asst
{
    class CharSkillConfig final : public SingletonHolder<CharSkillConfig>, public AbstractConfig
    {
    public:
        virtual ~CharSkillConfig() override = default;

        enum class Language
        {
            ZH_CN,
        };

        std::string get_skill_name_by_pos(const std::string& char_name, int pos, Language language = Language::ZH_CN);

    protected:
        virtual bool parse(const json::value& json) override;

    private:
        std::unordered_map<Language, std::unordered_map<std::string, std::vector<std::string>>>
            m_skill_name; // [language][skill_id] -> skills_name
    };

    inline static auto& CharSkill = CharSkillConfig::get_instance();
}// namespace asst
