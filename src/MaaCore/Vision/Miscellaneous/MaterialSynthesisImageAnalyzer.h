#pragma once

#include "Vision/VisionHelper.h"

namespace asst
{
class MaterialSynthesisImageAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~MaterialSynthesisImageAnalyzer() override = default;

    bool analyze();

    const MatchRect& get_result() const noexcept { return m_result; }

private:
    static double color_difference(const cv::Scalar& lhs, const cv::Scalar& rhs);
    static cv::Rect center_rect(const cv::Mat& image, int width = 80, int height = 40);

    MatchRect m_result;
};
}
