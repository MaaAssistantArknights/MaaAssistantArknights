#pragma once

#include "AbstractConfiger.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "AsstTypes.h"
#include "AsstBattleDef.h"

namespace asst
{
    // 干员信息，战斗相关
    struct CombatRecruitOperInfo
    {
        std::string name;
        BattleRole role;
        int level = 0;
        std::string subProfession;  // TODO: 改成枚举
        std::string position; // 高台--RANGED, 地面--MELEE  // TODO: 改成 TilePack::BuildableType 枚举
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
