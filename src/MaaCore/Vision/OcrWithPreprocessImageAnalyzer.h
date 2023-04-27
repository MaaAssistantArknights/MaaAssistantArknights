#pragma once
#include "OcrImageAnalyzer.h"

namespace asst
{
    class OcrWithPreprocessImageAnalyzer : public AbstractImageAnalyzer, public OCRerConfig
    {
    public:
        using Result = OcrImageAnalyzer::Result;
        using ResultOpt = std::optional<Result>;

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~OcrWithPreprocessImageAnalyzer() override = default;

        ResultOpt analyze() const;
        // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
        const auto& get_result() const noexcept { return m_result; }

    protected:
        virtual void _set_roi(const Rect& roi) override { set_roi(roi); }

    private:
        // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
        mutable Result m_result;
    };

}
