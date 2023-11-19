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
        std::vector<int> group_id = {};              // 干员组id,允许一个干员存在于多个干员组
        std::unordered_map<int, int> order_in_group; // 干员在干员组内的顺序 干员组id:干员组内顺序
        int recruit_priority = 0;                    // 招募优先级 (0-1000)
        int promote_priority = 0;                    // 晋升优先级 (0-1000)
        int recruit_priority_when_team_full = 0;     // 队伍满时的招募优先级 (0-1000)
        int promote_priority_when_team_full = 0;     // 队伍满时的晋升优先级 (0-1000)
        std::vector<std::pair<int, int>> recruit_priority_offset; // [deprecated]
        bool offset_melee = false;                                // [deprecated]
        bool is_key = false;                                      // 是否为核心干员
        bool is_start = false;                                    // 是否为开局干员
        std::vector<RecruitPriorityOffset> recruit_priority_offsets;
        bool is_alternate = false; // 是否后备干员 (允许重复招募、划到后备干员时不再往右划动)
        int skill = 0;             // 使用几技能
        int alternate_skill = 0; // 当没有指定技能时使用的备选技能，一般是6星干员未精二且精二后使用3技能时才需要指定
        battle::SkillUsage skill_usage = battle::SkillUsage::Possibly;           // 技能使用模式
        int skill_times = 1;                                                     // 技能使用次数
        battle::SkillUsage alternate_skill_usage = battle::SkillUsage::Possibly; // 备选技能使用模式
        int alternate_skill_times = 1;                                           // 备选技能使用次数
        int auto_retreat = 0;                                                    // 部署几秒后自动撤退
    };

    class RoguelikeRecruitConfig final : public SingletonHolder<RoguelikeRecruitConfig>, public AbstractConfig
    {
    public:
        virtual ~RoguelikeRecruitConfig() override = default;

        const RoguelikeOperInfo& get_oper_info(const std::string& theme, const std::string& name) noexcept;
        // 获取该肉鸽内用到的干员组[干员组1,干员组2,...]
        const std::vector<std::string> get_group_info(const std::string& theme) const noexcept;
        const std::vector<RecruitPriorityOffset> get_team_complete_info(const std::string& theme) const noexcept;
        std::vector<int> get_group_id(const std::string& theme, const std::string& name) const noexcept;

    protected:
        virtual bool parse(const json::value& json) override;

        void clear(const std::string& theme);

        std::unordered_map<std::string, std::unordered_map<std::string, RoguelikeOperInfo>> m_all_opers;
        std::unordered_map<std::string, std::vector<std::string>> m_oper_groups;
        std::unordered_map<std::string, std::vector<RecruitPriorityOffset>> m_team_complete_comdition;
    };

    inline static auto& RoguelikeRecruit = RoguelikeRecruitConfig::get_instance();
}
