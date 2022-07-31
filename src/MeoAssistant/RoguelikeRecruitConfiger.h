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
    struct RoguelikeOperInfo
    {
        std::string name;
        int skill = 0;
        int alternate_skill = 0;
        BattleSkillUsage skill_usage = BattleSkillUsage::Possibly;
        BattleSkillUsage alternate_skill_usage = BattleSkillUsage::Possibly;
    };
    class RoguelikeRecruitConfiger : public AbstractConfiger
    {
    public:
        virtual ~RoguelikeRecruitConfiger() override = default;

        const RoguelikeOperInfo& get_oper_info(const std::string& name) const noexcept;
        const auto& get_oper_order() const noexcept { return m_ordered_all_opers_name; }

    protected:
        virtual bool parse(const json::value& json) override;

        std::unordered_map<std::string, RoguelikeOperInfo> m_all_opers;
        std::vector<std::string> m_ordered_all_opers_name;
    };
}
