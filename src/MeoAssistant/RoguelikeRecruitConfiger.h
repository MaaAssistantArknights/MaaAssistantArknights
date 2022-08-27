#pragma once

#include "AbstractConfiger.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "AsstBattleDef.h"
#include "AsstTypes.h"

namespace asst
{
    // 干员信息，战斗相关
    struct RoguelikeOperInfo
    {
        std::string name;
        int recruit_priority = 0;  // 招募优先级 (0-1000)
        int promote_priority = 0;  // 晋升优先级 (0-1000)
        bool is_alternate = false; // 是否后备干员 (允许重复招募、划到后备干员时不再往右划动)
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
