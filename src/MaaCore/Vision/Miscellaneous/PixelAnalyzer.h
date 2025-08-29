#pragma once

#include "Vision/VisionHelper.h"

namespace asst
{
class PixelAnalyzer : public VisionHelper
{
public:
    enum class Filter
    {
        GRAY,
        RGB,
        HSV
    };

    using Result = Point;
    using ResultsVec = std::vector<Result>;
    using ResultsVecOpt = std::optional<ResultsVec>;

    using VisionHelper::VisionHelper;
    virtual ~PixelAnalyzer() override = default;

    ResultsVecOpt analyze();

    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    [[nodiscard]] const auto& get_result() const noexcept { return m_result; }

    void set_filter(const Filter filter) { m_filter = filter; }

    void set_gray_lb(const int gray_lb) { m_gray_lb = gray_lb; }

    void set_gray_ub(const int gray_ub) { m_ub = gray_ub; }

    void set_lb(const std::vector<int>& lb)
    {
        if (lb.size() == 3) {
            m_lb = cv::Scalar(lb[0], lb[1], lb[2]);
        }
    }

    void set_ub(const std::vector<int>& ub)
    {
        if (ub.size() == 3) {
            m_ub = cv::Scalar(ub[0], ub[1], ub[2]);
        }
    }

private:
    Filter m_filter = Filter::GRAY;
    int m_gray_lb = 250;
    int m_gray_ub = 255;
    cv::Scalar m_lb = { 0, 0, 0 };
    cv::Scalar m_ub = { 0, 0, 0 };
    ResultsVec m_result;
};
}
