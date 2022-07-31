#pragma once
#include "AbstractConfiger.h"

#include <unordered_map>
#include <unordered_set>

#include "AsstInfrastDef.h"

namespace asst
{
    class InfrastConfiger : public AbstractConfiger
    {
    public:
        virtual ~InfrastConfiger() override = default;

        auto get_templ_required() const noexcept -> const std::unordered_set<std::string>&
        {
            return m_templ_required;
        }
        auto get_skills(const std::string& facility_name) const -> const std::unordered_map<std::string, infrast::Skill>&
        {
            return m_skills.at(facility_name);
        }
        auto get_skills_group(const std::string& facility) const -> const std::vector<infrast::SkillsGroup>&
        {
            return m_skills_groups.at(facility);
        }
        auto get_facility_info(const std::string& facility) const -> const infrast::Facility&
        {
            return m_facilities_info.at(facility);
        }
        auto get_facility_info() const noexcept -> const std::unordered_map<std::string, infrast::Facility>&
        {
            return m_facilities_info;
        }

    protected:
        virtual bool parse(const json::value& json) override;

        // 所有基建技能，key：设施名，value：map<技能id，技能>
        std::unordered_map<std::string, std::unordered_map<std::string, infrast::Skill>> m_skills;
        // 所有设施信息，key：设施名，value：信息
        std::unordered_map<std::string, infrast::Facility> m_facilities_info;
        // 所有加成技能组
        std::unordered_map<std::string, std::vector<infrast::SkillsGroup>> m_skills_groups;

        // 需要加载的模板
        std::unordered_set<std::string> m_templ_required;
    };
}
