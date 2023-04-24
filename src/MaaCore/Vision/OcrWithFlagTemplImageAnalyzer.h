#pragma once
#include "Config/MatchImageAnalyzerConfig.h"
#include "Config/OcrImageAnalyzerConfig.h"
#include "OcrImageAnalyzer.h"

namespace asst
{
    class OcrWithFlagTemplImageAnalyzer : public AbstractImageAnalyzer,
                                          public OcrImageAnalyzerConfig,
                                          public MatchImageAnalyzerConfig
    {
    public:
        using Result = OcrImageAnalyzer::Result;
        using ResultsVec = OcrImageAnalyzer::ResultsVec;
        using ResultsVecOpt = OcrImageAnalyzer::ResultsVecOpt;

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~OcrWithFlagTemplImageAnalyzer() override = default;

        void set_task_info(const std::string& templ_task_name, const std::string& ocr_task_name);
        void set_flag_rect_move(Rect flag_rect_move);

        const ResultsVecOpt& analyze();

        void sort_results_by_horizontal();
        void sort_results_by_vertical();
        void sort_results_by_score();
        void sort_results_by_required();

        const ResultsVecOpt& result() const noexcept { return m_result; }

    protected:
        // from Config
        virtual void _set_roi(const Rect& roi) override { std::ignore = roi; }

    protected:
        using MatchImageAnalyzerConfig::set_task_info;
        using OcrImageAnalyzerConfig::set_task_info;

    private:
        Rect m_flag_rect_move;
        ResultsVecOpt m_result;
    };
}
