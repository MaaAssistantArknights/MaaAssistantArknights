#pragma once
#include "AbstractImageAnalyzer.h"
#include "StageDropsConfiger.h"

namespace asst
{
    class StageDropsImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~StageDropsImageAnalyzer() = default;

        virtual bool analyze() override;

    protected:
        bool analyze_stage_name();
        bool analyze_stars();
        bool analyze_difficulty();
        bool analyze_baseline();
        bool analyze_drops();

        int match_quantity(const Rect& roi);
        StageDropType match_droptype(const Rect& roi);
        std::string match_item(const Rect& roi, StageDropType type, int index, int size);

        std::string m_stage_name;
        StageDifficulty m_difficulty;
        std::vector<std::pair<Rect, StageDropType>> m_baseline;
        std::unordered_map<std::string, int> m_drops;
        int m_stars;
    };
}
