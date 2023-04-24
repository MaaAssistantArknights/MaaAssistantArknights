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

        const ResultsVecOpt& analyze();

        void sort_results_by_horizontal();
        void sort_results_by_vertical();
        void sort_results_by_score();
        void sort_results_by_required();

        const ResultsVecOpt& result() const noexcept { return m_result; }

    protected:
        virtual void _set_roi(const Rect& roi) override { set_roi(roi); }

    protected:
        void postproc_rect_(Result& res);
        void postproc_trim_(Result& res);
        void postproc_equivalence_(Result& res);
        void postproc_replace_(Result& res);

        bool filter_and_replace_by_required_(Result& res);

    private:
        ResultsVecOpt m_result;
    };
} // namespace asst
