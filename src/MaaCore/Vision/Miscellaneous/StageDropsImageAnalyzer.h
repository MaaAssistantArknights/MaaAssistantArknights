#pragma once
#include "Vision/AbstractImageAnalyzer.h"
#include "Config/Miscellaneous/StageDropsConfig.h"

namespace asst
{
    class StageDropsImageAnalyzer final : public AbstractImageAnalyzer
    {
        static constexpr const char* LMD_ID = "4001";

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~StageDropsImageAnalyzer() override = default;

        virtual bool analyze() override;

        StageKey get_stage_key() const;
        int get_stars() const noexcept;

        // <drop_type, <item_id, quantity>>
        const auto& get_drops() const noexcept { return m_drops; }

    protected:
        bool analyze_stage_code();
        bool analyze_stars();
        bool analyze_difficulty();
        bool analyze_baseline();
        bool analyze_drops();

        int match_quantity(const Rect& roi, bool use_word_model = false);
        StageDropType match_droptype(const Rect& roi);
        std::string match_item(const Rect& roi, StageDropType type, int index, int size);

        std::string m_stage_code;
        StageDifficulty m_difficulty = StageDifficulty::Normal;
        int m_stars = 0;
        std::vector<std::pair<Rect, StageDropType>> m_baseline;
        // <drop_type, <item_id, quantity>>
        std::vector<StageDropInfo> m_drops;
    };
}
