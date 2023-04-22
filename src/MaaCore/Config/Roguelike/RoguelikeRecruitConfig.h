#pragma once

#include "Config/AbstractConfig.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"

namespace asst
{
    // 招募priority影响因子
    struct RecruitPriorityOffset
    {
        std::vector<std::string> groups; // 造成优先级改变的干员组
        int threshold = 0;               // 已拥有组内干员数量阈值，达到此阈值时应用优先级改变
        bool is_less = false;            // 高于阈值时改变还是低于阈值时改变
        int offset = 0;                  // 优先级改变的大小
    };
    // 干员信息，战斗相关
    struct RoguelikeOperInfo
    {
        std::string name;
        int group_id = 0;                                         // 干员组id
        int recruit_priority = 0;                                 // 招募优先级 (0-1000)
        int promote_priority = 0;                                 // 晋升优先级 (0-1000)
        int recruit_priority_when_team_full = 0;                  // 队伍满时的招募优先级 (0-1000)
        int promote_priority_when_team_full = 0;                  // 队伍满时的晋升优先级 (0-1000)
        std::vector<std::pair<int, int>> recruit_priority_offset; // [deprecated]
        bool offset_melee = false;                                // [deprecated]
        bool is_key = false;                                      // 是否为核心干员
        bool is_start = false;                                    // 是否为开局干员
        std::vector<RecruitPriorityOffset> recruit_priority_offsets;
        bool is_alternate = false; // 是否后备干员 (允许重复招募、划到后备干员时不再往右划动)
        int skill = 0;
        int alternate_skill = 0;
        battle::SkillUsage skill_usage = battle::SkillUsage::Possibly;
        battle::SkillUsage alternate_skill_usage = battle::SkillUsage::Possibly;
    };

    class RoguelikeRecruitConfig final : public SingletonHolder<RoguelikeRecruitConfig>, public AbstractConfig
    {
    public:
        virtual ~RoguelikeRecruitConfig() override = default;

        const RoguelikeOperInfo& get_oper_info(const std::string& theme, const std::string& name) noexcept;
        const std::vector<std::pair<int, int>> get_role_info(const std::string& theme,
                                                             const battle::Role& role) const noexcept; // [deprecated]
        const std::vector<std::string> get_group_info(const std::string& theme) const noexcept;
        const std::vector<RecruitPriorityOffset> get_team_complete_info(const std::string& theme) const noexcept;
        int get_group_id(const std::string& theme, const std::string& name) const noexcept;

    protected:
        virtual bool parse(const json::value& json) override;

        void clear();

        std::unordered_map<std::string, std::unordered_map<std::string, RoguelikeOperInfo>> m_all_opers;
        std::unordered_map<std::string, std::unordered_map<battle::Role, std::vector<std::pair<int, int>>>>
            m_role_offset_map; // [deprecated]
        std::unordered_map<std::string, std::vector<std::string>> m_oper_groups;
        std::unordered_map<std::string, std::vector<RecruitPriorityOffset>> m_team_complete_comdition;
    };

    inline static auto& RoguelikeRecruit = RoguelikeRecruitConfig::get_instance();
}
