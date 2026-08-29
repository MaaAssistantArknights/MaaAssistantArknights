#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "AsstTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"

namespace asst::infrast
{
using OperatorIds = std::unordered_set<std::string>;

enum class FacilityPlanMode
{
    Default,
    Custom,
    Rotation,
};

enum class FacilityStep
{
    DormPrepare,
    DormFill,
    MfgInspect,
    Mfg,
    Trade,
    Power,
    Office,
    ControlForce,
    Reception,
    Processing,
    Training,
    AssistantChange,
};

inline std::optional<std::vector<FacilityStep>>
    build_facility_plan(FacilityPlanMode mode, const std::vector<std::string>& facilities)
{
    const auto to_step = [](const std::string& facility) -> std::optional<FacilityStep> {
        if (facility == "Dorm") {
            return FacilityStep::DormPrepare;
        }
        if (facility == "Mfg") {
            return FacilityStep::Mfg;
        }
        if (facility == "Trade") {
            return FacilityStep::Trade;
        }
        if (facility == "Power") {
            return FacilityStep::Power;
        }
        if (facility == "Office") {
            return FacilityStep::Office;
        }
        if (facility == "Control") {
            return FacilityStep::ControlForce;
        }
        if (facility == "Reception") {
            return FacilityStep::Reception;
        }
        if (facility == "Processing") {
            return FacilityStep::Processing;
        }
        if (facility == "Training") {
            return FacilityStep::Training;
        }
        if (facility == "AssistantChange") {
            return FacilityStep::AssistantChange;
        }
        return std::nullopt;
    };

    for (const auto& facility : facilities) {
        if (!to_step(facility)) {
            return std::nullopt;
        }
    }

    if (mode != FacilityPlanMode::Default) {
        const std::unordered_set<std::string> rotation_skips = { "Dorm", "Power", "Office", "Control" };
        std::vector<FacilityStep> result;
        result.reserve(facilities.size());
        for (const auto& facility : facilities) {
            if (mode == FacilityPlanMode::Rotation && rotation_skips.contains(facility)) {
                continue;
            }
            result.emplace_back(*to_step(facility));
        }
        return result;
    }

    // 常规模式中的 facility 是启用集合，数组顺序与重复项不参与调度。
    const std::unordered_set<std::string> enabled(facilities.begin(), facilities.end());
    std::vector<FacilityStep> result;
    if (enabled.contains("Trade") && !enabled.contains("Mfg")) {
        result.emplace_back(FacilityStep::MfgInspect);
    }
    if (enabled.contains("Dorm")) {
        result.emplace_back(FacilityStep::DormPrepare);
    }
    if (enabled.contains("Power")) {
        result.emplace_back(FacilityStep::Power);
    }
    if (enabled.contains("Office")) {
        result.emplace_back(FacilityStep::Office);
    }
    if (enabled.contains("Control")) {
        result.emplace_back(FacilityStep::ControlForce);
    }
    if (enabled.contains("Mfg")) {
        result.emplace_back(FacilityStep::Mfg);
    }
    if (enabled.contains("Trade")) {
        result.emplace_back(FacilityStep::Trade);
    }
    if (enabled.contains("Reception")) {
        result.emplace_back(FacilityStep::Reception);
    }
    if (enabled.contains("Dorm")) {
        result.emplace_back(FacilityStep::DormFill);
    }
    if (enabled.contains("Processing")) {
        result.emplace_back(FacilityStep::Processing);
    }
    if (enabled.contains("Training")) {
        result.emplace_back(FacilityStep::Training);
    }
    if (enabled.contains("AssistantChange")) {
        result.emplace_back(FacilityStep::AssistantChange);
    }
    return result;
}

inline OperatorIds intersect_operator_ids(const std::vector<OperatorIds>& candidates)
{
    auto iter = std::ranges::find_if(candidates, [](const OperatorIds& ids) { return !ids.empty(); });
    if (iter == candidates.end()) {
        return {};
    }

    OperatorIds result = *iter;
    for (++iter; iter != candidates.end(); ++iter) {
        if (iter->empty()) {
            continue;
        }
        std::erase_if(result, [&](const std::string& id) { return !iter->contains(id); });
    }
    return result;
}

inline bool operator_id_matches_candidates(const OperatorIds& candidates, std::string_view recognized_id)
{
    return !recognized_id.empty() && (candidates.empty() || candidates.contains(std::string(recognized_id)));
}

struct OperatorSelection
{
    OperatorIds operator_ids;
    OperatorIds pending_operator_ids;

    void commit_pending()
    {
        operator_ids.insert(pending_operator_ids.begin(), pending_operator_ids.end());
        pending_operator_ids.clear();
    }

    void discard_pending() { pending_operator_ids.clear(); }

    void clear_operator_selection()
    {
        operator_ids.clear();
        pending_operator_ids.clear();
    }
};

struct Facility
{
    std::string id;
    std::vector<std::string> products;
    int max_num_of_opers = 0;
};

struct FacilityInfo
{
    Rect rect;
    int level = 0;
};

struct TaskData : OperatorSelection
{
    std::unordered_map<std::string, std::vector<FacilityInfo>> facilities;
    std::unordered_set<int> gold_station_indices;

    int dormitory_capacity = 0;
    int dormitory_level_sum = 0;
    int gold_station_num = 0;
    int trading_station_num = 0;
    int power_station_num = 0;
    int virtual_power_station_num = 0;
    int total_station_level = 0;
    int workbench_num = 0;

    void refresh_derived_state()
    {
        const std::unordered_set<std::string> workbenches = {
            "char_285_medic2",  "char_286_cast3",   "char_376_therex",
            "char_4000_jnight", "char_4093_frston", "char_4136_phonor",
        };
        workbench_num = static_cast<int>(
            std::ranges::count_if(operator_ids, [&](const std::string& id) { return workbenches.contains(id); }));
        virtual_power_station_num = power_station_num;
        if (operator_ids.contains("char_1027_greyy2")) {
            ++virtual_power_station_num;
        }
        if (operator_ids.contains("char_416_zumama") && operator_ids.contains("char_285_medic2")) {
            virtual_power_station_num += 2;
        }
    }

    void commit_pending()
    {
        OperatorSelection::commit_pending();
        refresh_derived_state();
    }

    void clear() { *this = {}; }
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
    std::unordered_set<std::string> operator_ids;      // 可能拥有该技能的干员 ID
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
    std::unordered_set<std::string> operator_ids;
    std::string operator_id;
    Rect rect;
    // 因为OCR识别名字比较费时间，所以仅在name_filter不为空（有识别名字需求）的时候才识别，否则仅保存图片但不识别
    cv::Mat name_img;
    cv::Mat facility_img;

    static std::unordered_set<std::string> intersect_operator_ids(const std::vector<Skill>& skills)
    {
        std::vector<OperatorIds> candidates;
        candidates.reserve(skills.size());
        for (const auto& skill : skills) {
            candidates.emplace_back(skill.operator_ids);
        }
        return infrast::intersect_operator_ids(candidates);
    }
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
    std::string face_hash;
    std::unordered_set<std::string> operator_ids;
    std::string operator_id;

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
