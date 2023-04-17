#pragma once
#include "OcrImageAnalyzer.h"

namespace asst
{
    class OcrWithPreprocessImageAnalyzer : public OcrImageAnalyzer
    {
    public:
        using Result = OcrImageAnalyzer::Result;
        using ResultsVec = OcrImageAnalyzer::ResultsVec;
        using ResultsVecOpt = OcrImageAnalyzer::ResultsVecOpt;

    public:
        using OcrImageAnalyzer::OcrImageAnalyzer;
        virtual ~OcrWithPreprocessImageAnalyzer() override = default;

        virtual const ResultsVecOpt& analyze() override;

        void set_threshold(int lower, int upper = 255);
        void set_split(bool split);
        void set_expansion(int expansion);

    protected:
        int m_threshold_lower = 140;
        int m_threshold_upper = 255;
        bool m_split = false;
        int m_expansion = 2;
    };
}
