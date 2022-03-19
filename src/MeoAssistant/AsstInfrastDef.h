#pragma once

#include "AsstTypes.h"

namespace asst
{
    namespace infrast
    {
        struct Facility
        {
            ::std::string id;
            ::std::vector<::std::string> products;
            int max_num_of_opers = 0;
        };

        enum class SmileyType
        {
            Invalid = -1,
            Rest,           // 休息完成，绿色笑脸
            Work,           // 工作中，黄色笑脸
            Distract        // 注意力涣散，红色哭脸
        };
        struct Smiley
        {
            SmileyType type = SmileyType::Invalid;
            Rect rect;
        };
        enum class Doing    // 正在做什么
        {
            Invalid = -1,
            Nothing,        // 什么都不在做，也不在休息也不在工作
            Resting,        // 休息中
            Working         // 工作中
        };

        struct Skill
        {
            ::std::string id;
            ::std::string templ_name;
            ::std::vector<::std::string> names;                                 // 很多基建技能是一样的，就是名字不同。所以一个技能id可能对应多个名字
            ::std::string desc;
            ::std::unordered_map<::std::string, double> efficient;              // 技能效率，key：产品名（赤金、经验书等）, value: 效率数值
            ::std::unordered_map<::std::string, ::std::string> efficient_regex; // 技能效率正则，key：产品名（赤金、经验书等）, value: 效率正则。如不为空，会先对正则进行计算，再加上efficient里面的值
            int max_num = INT_MAX;                                              // 最多选几个该技能

            bool operator==(const Skill& skill) const noexcept
            {
                return id == skill.id;
            }
        };
    }
}

namespace std
{
    template <>
    class hash<asst::infrast::Skill>
    {
    public:
        size_t operator()(const asst::infrast::Skill& skill) const
        {
            return ::std::hash<std::string>()(skill.id);
        }
    };
}

namespace asst
{
    namespace infrast
    {
        struct BattleRealTimeOper
        {
            ::std::string face_hash;             // 有些干员的技能是完全一样的，做个hash区分一下不同干员
            ::std::string name_hash;             // 预留
            Smiley smiley;
            double mood_ratio = 0;          // 心情进度条的百分比
            Doing doing = Doing::Invalid;
            bool selected = false;          // 干员是否已被选择（蓝色的选择框）
            ::std::unordered_set<Skill> skills;
            Rect rect;
        };

        struct SkillsComb
        {
            SkillsComb() = default;
            SkillsComb(std::unordered_set<Skill> skill_vec)
            {
                skills = ::std::move(skill_vec);
                for (const auto& s : skills) {
                    for (const auto& [key, value] : s.efficient) {
                        efficient[key] += value;
                    }
                    for (const auto& [key, reg] : s.efficient_regex) {
                        efficient_regex[key] += "+(" + reg + ")";
                    }
                }
            }
            bool operator==(const SkillsComb& rhs) const
            {
                return skills == rhs.skills;
            }

            ::std::string desc;
            ::std::unordered_set<Skill> skills;
            ::std::unordered_map<std::string, double> efficient;
            ::std::unordered_map<std::string, ::std::string> efficient_regex;

            ::std::string name_hash;
            bool hash_filter = false;
            ::std::unordered_map<std::string, ::std::string> possible_hashs; // 限定只允许某些hash匹配的某些干员。若hash不相同，即使技能匹配了也不可用。hashs若为空，则不生效
        };
        // 基建技能组
        struct SkillsGroup
        {
            ::std::string desc;                                   // 文字介绍，实际不起作用
            ::std::unordered_map<std::string, int> conditions;    // 技能组合可用条件，例如：key 发电站数量，value 3
            ::std::vector<SkillsComb> necessary;                  // 必选技能。这里面的缺少任一，则该技能组合不可用
            ::std::vector<SkillsComb> optional;                   // 可选技能。
            bool allow_external = false;                          // 当干员数没满3个的时候，是否允许补充外部干员
        };

        enum class WorkMode
        {
            Invalid = -1,
            Gentle,     // 温和换班模式：会对干员人数不满的设施进行换班，计算单设施内最优解，尽量不破坏原有的干员组合；即若设施内干员是满的，则不对该设施进行换班
            Aggressive, // 激进换班模式：会对每一个设施进行换班，计算单设施内最优解，但不会将其他设施中的干员替换过来；即按工作状态排序，仅选择前面的干员
            Extreme     // 偏激换班模式：会对每一个设施进行换班，计算全局的单设施内最优解，为追求更高效率，会将其他设施内的干员也替换过来；即按技能排序，计算所有拥有该设施技能的干员效率，无论他在不在其他地方工作
        };
    }
}
