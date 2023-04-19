#pragma once
#include "MatchImageAnalyzer.h"

namespace asst
{
    class MultiMatchImageAnalyzer : public MatchImageAnalyzer
    {
    public:
        using Result = MatchImageAnalyzer::Result;
        using ResultsVec = std::vector<Result>;
        using ResultsVecOpt = std::optional<ResultsVec>;

    public:
        using MatchImageAnalyzer::MatchImageAnalyzer;
        virtual ~MultiMatchImageAnalyzer() override = default;

        const ResultsVecOpt& analyze();

        void sort_results_by_horizontal(); // 按位置排序，左上角的排在前面，右上角在左下角前面
        void sort_results_by_vertical();   // 按位置排序，左上角的排在前面，左下角在右上角前面
        void sort_results_by_score();      // 按分数排序，得分最高的在前面

    protected:
        virtual bool multi_match_templ(const cv::Mat templ);

        ResultsVecOpt m_multi_result;
    };
}
