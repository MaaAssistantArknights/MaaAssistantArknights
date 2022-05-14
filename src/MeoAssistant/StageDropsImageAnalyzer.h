#pragma once
#include "AbstractImageAnalyzer.h"

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
        bool analyze_drops();

        std::string m_stage_name;
        int m_stars;
    };
}
