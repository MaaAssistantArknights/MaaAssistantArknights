#pragma once
#include "AbstractConfiger.h"

#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
        ExpAndLMB,
        Normal,
        Extra,
        Funriture,
        Special
    };
    struct StageInfo
    {
        std::string stage_id;
        int ap_cost = 0;
        std::unordered_map<StageDropType, std::vector<std::string>> drops;
    };

    class StageDropsConfiger final : public AbstractConfiger
    {
    public:
        using AbstractConfiger::AbstractConfiger;
        virtual ~StageDropsConfiger() = default;

        const auto& get_stage_info(const std::string& code, StageDifficulty difficulty = StageDifficulty::Normal) const
        {
            return m_stage_info.at(StageKey{ code, difficulty });
        }
        const auto& get_all_stage_code() const
        {
            return m_all_stage_code;
        }
        const auto& get_all_item_id() const
        {
            return m_all_item_id;
        }

    protected:
        virtual bool parse(const json::value& json) override;

        std::unordered_set<std::string> m_all_stage_code;
        std::unordered_set<std::string> m_all_item_id;
        std::unordered_map<StageKey, StageInfo, StageKeyHasher> m_stage_info;
    };
}
