#pragma once
#include "AbstractConfiger.h"

#include <unordered_map>
#include <unordered_set>

#include "AsstDef.h"

namespace asst {
    class InfrastConfiger : public AbstractConfiger
    {
    public:
        virtual ~InfrastConfiger() = default;

        const std::unordered_set<std::string>& get_templ_required() const noexcept {
            return m_templ_required;
        }
        const std::unordered_map<std::string, InfrastSkill>&
            get_skills(const std::string& facility_name) const
        {
            return m_skills.at(facility_name);
        }
        const std::vector<InfrastSkillsGroup>& get_skills_group() const {
            return m_skills_groups;
        }

    protected:
        virtual bool parse(const json::value& json) override;

        std::unordered_map<std::string, std::unordered_map<std::string, InfrastSkill>> m_skills;
        std::unordered_set<std::string> m_templ_required;

        std::vector<InfrastSkillsGroup> m_skills_groups;
    };
}
