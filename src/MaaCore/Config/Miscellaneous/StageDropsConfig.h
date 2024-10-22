#pragma once
#include "Config/AbstractConfig.h"

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace asst
{
enum class StageDifficulty
{
    Normal,
    Tough
};

inline std::string enum_to_string(StageDifficulty algo)
{
    static const std::unordered_map<StageDifficulty, std::string> algorithm_map = {
        { StageDifficulty::Normal, "Normal" },
        { StageDifficulty::Tough, "Tough" }
    };
    if (auto it = algorithm_map.find(algo); it != algorithm_map.end()) {
        return it->second;
    }
    return "Invalid";
}

struct StageKey
{
    std::string code;
    StageDifficulty difficulty = StageDifficulty::Normal;

    bool operator==(const StageKey& other) const { return code == other.code && difficulty == other.difficulty; }
};

struct StageKeyHasher
{
    size_t operator()(const StageKey& key) const
    {
        return std::hash<std::string>()(key.code) ^ (static_cast<size_t>(key.difficulty) << 1);
    }
};

enum class StageDropType
{
    Unknown,
    ExpAndLMB,
    Normal,
    Extra,
    Furniture, // 家具
    Special,   // 稀有材料、周年庆箱子等
    Sanity,    // 理智返还
    Reward     // 报酬（合成玉，仅剿灭）
};

struct StageInfo
{
    std::string stage_id;
    int ap_cost = 0;
    std::unordered_map<StageDropType, std::vector<std::string>> drops;
};

struct StageDropInfo
{
    StageDropType drop_type = StageDropType::Unknown;
    std::string drop_type_name;
    std::string item_id;
    std::string item_name;
    int quantity = 0;
};

class StageDropsConfig final : public SingletonHolder<StageDropsConfig>, public AbstractConfig
{
public:
    virtual ~StageDropsConfig() override = default;

    const auto& get_stage_info(const std::string& code, StageDifficulty difficulty) const
    {
        StageKey key { code, difficulty };
        if (auto find_iter = m_stage_info.find(key); find_iter != m_stage_info.end()) {
            return find_iter->second;
        }
        else {
            static StageInfo empty_info;
            return empty_info;
        }
    }

    const auto& get_all_stage_code() const { return m_all_stage_code; }

    const auto& get_all_item_id() const { return m_all_item_id; }

    void append_drops(const StageKey& stage_key, StageDropType type, std::string item_id)
    {
        m_stage_info[stage_key].drops[type].emplace_back(std::move(item_id));
    }

protected:
    virtual bool parse(const json::value& json) override;

    std::unordered_set<std::string> m_all_stage_code;
    std::unordered_set<std::string> m_all_item_id;
    std::unordered_map<StageKey, StageInfo, StageKeyHasher> m_stage_info;
};

inline static auto& StageDrops = StageDropsConfig::get_instance();
} // namespace asst
