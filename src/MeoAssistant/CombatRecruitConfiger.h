#pragma once

#include "AbstractConfiger.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "AsstTypes.h"

namespace asst
{
    // 干员信息，战斗相关
    struct CombatRecruitOperInfo
    {
        std::string name;
        // ["MEDIC","WARRIOR","SPECIAL","SNIPER","PIONEER","TANK","SUPPORT","CASTER"]
        std::string type;
        int level = 0;
        std::string subProfession;
        std::string position; // 高台--RANGED, 地面--MELEE
    };
    class CombatRecruitConfiger : public AbstractConfiger
    {
    public:
        virtual ~CombatRecruitConfiger() = default;

        const std::unordered_map<std::string, CombatRecruitOperInfo>& get_all_opers() const noexcept
        {
            return m_all_opers;
        }

        const std::vector<std::string>& get_all_opers_name() const noexcept
        {
            return m_all_opers_name;
        }

    protected:
        virtual bool parse(const json::value& json) override;

        std::unordered_map<std::string, CombatRecruitOperInfo> m_all_opers;
        std::vector<std::string> m_all_opers_name;
    };
}
