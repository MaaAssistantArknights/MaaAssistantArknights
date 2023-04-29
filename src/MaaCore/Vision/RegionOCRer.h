#pragma once
#include "OCRer.h"

namespace asst
{
    class RegionOCRer : public VisionHelper, public OCRerConfig
    {
    public:
        using Result = OCRer::Result;
        using ResultOpt = std::optional<Result>;

    public:
        using VisionHelper::VisionHelper;
        virtual ~RegionOCRer() override = default;

        ResultOpt analyze() const;
        // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
        const auto& get_result() const noexcept { return m_result; }

    protected:
        using OCRerConfig::set_without_det;
        virtual void _set_roi(const Rect& roi) override { set_roi(roi); }

        void bin_left_trim(cv::Mat& bin) const;
        void bin_right_trim(cv::Mat& bin) const;

    private:
        // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
        mutable Result m_result;
    };
}
