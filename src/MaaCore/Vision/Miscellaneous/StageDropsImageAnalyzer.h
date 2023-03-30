#pragma once
#include "Config/Miscellaneous/StageDropsConfig.h"
#include "Vision/AbstractImageAnalyzer.h"

#include <optional>

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
        // 落叶殇火 活动（异格夜刀）, act24side, 2023-03
        bool analyze_drops_for_CF();

        int match_quantity(const Rect& roi, const std::string& item, bool use_word_model = false);
        std::optional<TextRect> match_quantity_string(const Rect& roi, bool use_word_model = false);
        std::optional<TextRect> match_quantity_string(const Rect& roi, const std::string& item,
                                                      bool use_word_model = false);
        static int quantity_string_to_int(const std::string& str);

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
