#pragma once
#include "OcrImageAnalyzer.h"

namespace asst
{
    class OcrWithPreprocessImageAnalyzer : public AbstractImageAnalyzer, public OcrImageAnalyzerConfig
    {
    public:
        using Result = OcrImageAnalyzer::Result;
        using ResultOpt = std::optional<Result>;

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~OcrWithPreprocessImageAnalyzer() override = default;

        const ResultOpt& analyze();
        const ResultOpt& result() const noexcept { return m_result; }

        void set_threshold(int lower, int upper = 255);
        void set_expansion(int expansion);
        // void set_split(bool split);

    protected:
        virtual void _set_roi(const Rect& roi) override { set_roi(roi); }

    protected:
        int m_threshold_lower = 140;
        int m_threshold_upper = 255;
        // bool m_split = false;
        int m_expansion = 2;

    private:
        ResultOpt m_result;
    };
}
