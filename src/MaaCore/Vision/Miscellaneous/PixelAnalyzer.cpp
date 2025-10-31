#include "PixelAnalyzer.h"

#include <ranges>

#include "Utils/Logger.hpp"
#include "MaaUtils/NoWarningCV.hpp"

using namespace asst;

bool PixelAnalyzer::analyze()
{
    const cv::Mat croppedImage = make_roi(m_image, m_roi);
    cv::Mat tempImage;
    cv::Mat binaryImage;
    switch (m_filter) {
    case Filter::GRAY:
        cv::cvtColor(croppedImage, tempImage, cv::COLOR_BGR2GRAY);
        cv::threshold(tempImage, binaryImage, m_gray_lb, m_gray_ub, cv::THRESH_BINARY);
        break;
    case Filter::RGB:
        cv::cvtColor(croppedImage, tempImage, cv::COLOR_BGR2RGB);
        cv::inRange(tempImage, m_lb, m_ub, binaryImage);
        break;
    case Filter::HSV:
        cv::cvtColor(croppedImage, tempImage, cv::COLOR_BGR2HSV);
        cv::inRange(tempImage, m_lb, m_ub, binaryImage);
        break;
    }
    std::vector<cv::Point> pixelPoints;
    cv::findNonZero(binaryImage, pixelPoints);

    if (pixelPoints.empty()) {
        return false;
    }

    auto transform_view = pixelPoints | std::views::transform([](const cv::Point& p) { return Point(p.x, p.y); });
    Result result(std::ranges::begin(transform_view), std::ranges::end(transform_view));

    if (m_log_tracing) {
        Log.trace("analyze_bright_points | num:", result.size());
    }

    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    m_result = std::move(result);
    return true;
}
