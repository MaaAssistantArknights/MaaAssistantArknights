#pragma once
#include "AbstractConfiger.h"

#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <ctime>

namespace asst
{
    enum class StageDifficulty
    {
        Normal,
        Tough
    };
    struct StageKey
    {
        std::string code;
        StageDifficulty difficulty = StageDifficulty::Normal;
        bool operator==(const StageKey& other) const
        {
            return code == other.code && difficulty == other.difficulty;
        }
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
        Furniture,  // 家具
        Special,    // 稀有材料、周年庆箱子等
        Sanity,     // 理智返还
        Reward      // 报酬（合成玉，仅剿灭）
    };
    struct StageInfo
    {
        std::string stage_id;
        int ap_cost = 0;
        unsigned long openTime = 0, closeTime = 0;
        std::unordered_map<StageDropType, std::vector<std::string>> drops;
    };
    struct StageDropInfo
    {
        StageDropType droptype = StageDropType::Unknown;
        std::string droptype_name;
        std::string item_id;
        std::string item_name;
        int quantity = 0;
    };

    class StageDropsConfiger final : public AbstractConfiger
    {
    public:
        using AbstractConfiger::AbstractConfiger;
        virtual ~StageDropsConfiger() = default;

        const auto& get_stage_info(const std::string& code, StageDifficulty difficulty) const
        {
            unsigned long current = (unsigned long)time(0)*1000;
            StageKey key{ code, difficulty };
            if (auto find_iter = m_stage_info.find(key);
                find_iter != m_stage_info.end() &&
                current >= find_iter->second.openTime &&
                current < find_iter->second.closeTime) {
                return find_iter->second;
            }
            else {
                static StageInfo empty_info;
                return empty_info;
            }
        }
        const auto& get_all_stage_code() const
        {
            return m_all_stage_code;
        }
        const auto& get_all_item_id() const
        {
            return m_all_item_id;
        }
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
}
