#pragma once

#include "AsstTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"

namespace asst::infrast
{
struct Facility
{
    std::string id;
    std::vector<std::string> products;
    int max_num_of_opers = 0;
};

enum class SmileyType
{
    Invalid = -1,
    Rest,    // 休息完成，绿色笑脸
    Work,    // 工作中，黄色笑脸
    Distract // 注意力涣散，红色哭脸
};

struct Smiley
{
    SmileyType type = SmileyType::Invalid;
    Rect rect;
};
enum class Doing // 正在做什么
{
    Invalid = -1,
    Nothing, // 什么都不在做，也不在休息也不在工作
    Resting, // 休息中
    Working  // 工作中
};

struct Skill
{
    std::string id;
    std::string templ_name;
    std::vector<std::string> names; // 很多基建技能是一样的，就是名字不同。所以一个技能id可能对应多个名字
    std::string desc;
    std::unordered_map<std::string, double> efficient; // 技能效率，key：产品名（赤金、经验书等）, value: 效率数值
    std::unordered_map<std::string, std::string>
        efficient_regex;                               // 技能效率正则，key：产品名（赤金、经验书等）, value: 效率正则。
                                                       // 如不为空，会先对正则进行计算，再加上efficient里面的值
    int max_num = INT_MAX;                             // 最多选几个该技能

    bool operator==(const Skill& skill) const noexcept { return id == skill.id; }
};
}

namespace std
{
template <>
struct hash<asst::infrast::Skill>
{
    size_t operator()(const asst::infrast::Skill& skill) const noexcept { return ::std::hash<std::string>()(skill.id); }
};
}

namespace asst::infrast
{
struct Oper
{
    std::string face_hash; // 有些干员的技能是完全一样的，做个hash区分一下不同干员
    Smiley smiley;
    double mood_ratio = 0; // 心情进度条的百分比
    Doing doing = Doing::Invalid;
    bool selected = false; // 干员是否已被选择（蓝色的选择框）
    std::unordered_set<Skill> skills;
    Rect rect;
    // 因为OCR识别名字比较费时间，所以仅在name_filter不为空（有识别名字需求）的时候才识别，否则仅保存图片但不识别
    cv::Mat name_img;
    cv::Mat facility_img;
};

struct SkillsComb
{
    SkillsComb() = default;

    SkillsComb(std::unordered_set<Skill> skill_vec)
    {
        skills = std::move(skill_vec);
        for (const auto& s : skills) {
            for (const auto& [key, value] : s.efficient) {
                efficient[key] += value;
            }
            for (const auto& [key, reg] : s.efficient_regex) {
                efficient_regex[key] += "+(" + reg + ")";
            }
        }
    }

    bool operator==(const SkillsComb& rhs) const { return skills == rhs.skills; }

    std::string desc;
    std::unordered_set<Skill> skills;
    std::unordered_map<std::string, double> efficient;
    std::unordered_map<std::string, std::string> efficient_regex;

    std::vector<std::string> name_filter;
    // 因为OCR识别名字比较费时间，所以仅在name_filter不为空（有识别名字需求）的时候才识别，否则仅保存图片但不识别
    cv::Mat name_img;
};

// 基建技能组
struct SkillsGroup
{
    std::string desc;                                // 文字介绍，实际不起作用
    std::unordered_map<std::string, int> conditions; // 技能组合可用条件，例如：key 发电站数量，value 3
    std::vector<SkillsComb> necessary;               // 必选技能。这里面的缺少任一，则该技能组合不可用
    std::vector<SkillsComb> optional;                // 可选技能。
    bool allow_external = false;                     // 当干员数没满3个的时候，是否允许补充外部干员
};

struct CustomRoomConfig
{
    enum class Product
    {
        Unknown,
        BattleRecord,
        PureGold,
        Dualchip,
        OriginiumShard,
        LMD,
        Orundum,
    };

    bool skip = false;
    // 是否使用干员编组
    bool use_operator_groups = false;
    // 干员编组列表
    std::unordered_map<std::string, std::vector<std::string>> operator_groups;
    // 自定干员
    std::vector<std::string> names;
    bool autofill = false;
    Product product = Product::Unknown;
    std::vector<std::string> candidates;
    int selected = 0;
    bool sort = false;
    std::vector<std::string> blacklist;
};

using CustomFacilityConfig = std::vector<CustomRoomConfig>;

struct CustomDronesConfig
{
    enum class Order
    {
        Pre,
        Post,
    };

    int index = 0;
    Order order = Order::Pre;
};
} // namespace asst::infrast
