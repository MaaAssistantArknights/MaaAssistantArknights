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
        const std::vector<InfrastSkillsGroup>& get_skills_group(const std::string& facility) const {
            return m_skills_groups.at(facility);
        }
        const InfrastFacilityInfo& get_facility_info(const std::string& facility) const {
            return m_facilities_info.at(facility);
        }
        const std::unordered_map<std::string, InfrastFacilityInfo>& get_facility_info() const noexcept {
            return m_facilities_info;
        }

    protected:
        virtual bool parse(const json::value& json) override;

        // 所有基建技能，key：设施名，value：map<技能id，技能>
        std::unordered_map<std::string, std::unordered_map<std::string, InfrastSkill>> m_skills;
        // 所有设施信息，key：设施名，value：信息
        std::unordered_map<std::string, InfrastFacilityInfo> m_facilities_info;
        // 所有加成技能组
        std::unordered_map<std::string, std::vector<InfrastSkillsGroup>> m_skills_groups;

        // 需要加载的模板
        std::unordered_set<std::string> m_templ_required;
    };
}
