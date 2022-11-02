#pragma once

#include "AbstractConfiger.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "Utils/AsstBattleDef.h"
#include "Utils/AsstTypes.h"

namespace asst
{
    // 干员信息，战斗相关
    struct RoguelikeOperInfo
    {
        std::string name;
        int recruit_priority = 0;                // 招募优先级 (0-1000)
        int promote_priority = 0;                // 晋升优先级 (0-1000)
        int recruit_priority_when_team_full = 0; // 队伍满时的招募优先级 (0-1000)
        int promote_priority_when_team_full = 0; // 队伍满时的晋升优先级 (0-1000)
        std::vector<std::pair<int, int>> recruit_priority_offset;
        bool offset_melee = false;
        bool is_alternate = false; // 是否后备干员 (允许重复招募、划到后备干员时不再往右划动)
        int skill = 0;
        int alternate_skill = 0;
        BattleSkillUsage skill_usage = BattleSkillUsage::Possibly;
        BattleSkillUsage alternate_skill_usage = BattleSkillUsage::Possibly;
    };

    class RoguelikeRecruitConfiger final : public SingletonHolder<RoguelikeRecruitConfiger>, public AbstractConfiger
    {
    public:
        virtual ~RoguelikeRecruitConfiger() override = default;

        const RoguelikeOperInfo& get_oper_info(const std::string& theme, const std::string& name) const noexcept;
        const std::vector<std::pair<int, int>> get_role_info(const std::string& theme,
                                                             const BattleRole& role) const noexcept;

    protected:
        virtual bool parse(const json::value& json) override;

        void clear();

        std::unordered_map<std::string, std::unordered_map<std::string, RoguelikeOperInfo>> m_all_opers;
        std::unordered_map<std::string, std::unordered_map<BattleRole, std::vector<std::pair<int, int>>>>
            m_role_offset_map;
    };

    inline static auto& RoguelikeRecruit = RoguelikeRecruitConfiger::get_instance();
}
