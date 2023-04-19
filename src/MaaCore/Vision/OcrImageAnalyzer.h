#pragma once
#include "AbstractImageAnalyzer.h"

#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/OcrPack.h"
#include "Vision/Config/OcrImageAnalyzerConfig.h"

namespace asst
{
    class OcrImageAnalyzer : public AbstractImageAnalyzer, public OcrImageAnalyzerConfig
    {
    public:
        using Result = OcrPack::Result;
        using ResultsVec = OcrPack::ResultsVec;
        using ResultsVecOpt = std::optional<ResultsVec>;

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~OcrImageAnalyzer() override = default;

        virtual const ResultsVecOpt& analyze();

        void sort_results_by_horizontal(); // 按位置排序，左上角的排在前面，右上角在左下角前面
        void sort_results_by_vertical();   // 按位置排序，左上角的排在前面，左下角在右上角前面
        void sort_results_by_score();      // 按分数排序，得分最高的在前面
        void sort_results_by_required();   // 按传入的需求数组排序，传入的在前面结果接在前面

        const ResultsVecOpt& result() const noexcept;

    protected:
        virtual void _set_roi(const Rect& roi) override;

        void postproc_rect_(Result& res);
        void postproc_trim_(Result& res);
        void postproc_equivalence_(Result& res);
        void postproc_replace_(Result& res);

        bool filter_and_replace_by_required_(Result& res);

    protected:
        ResultsVecOpt m_result;
    };
} // namespace asst
