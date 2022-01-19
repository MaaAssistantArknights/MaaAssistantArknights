#include "BattlePerspectiveImageAnalyzer.h"

#include "Logger.hpp"

bool asst::BattlePerspectiveImageAnalyzer::analyze()
{
    home_analyze();
    return placed_analyze();
}

void asst::BattlePerspectiveImageAnalyzer::set_src_homes(std::vector<Rect> src_homes)
{
    m_src_homes = std::move(src_homes);
}

asst::Point asst::BattlePerspectiveImageAnalyzer::get_nearest_point() const noexcept
{
    return m_nearest_point;
}

const std::vector<asst::Rect>& asst::BattlePerspectiveImageAnalyzer::get_homes() const noexcept
{
    return m_homes.empty() ? m_src_homes : m_homes;
}

bool asst::BattlePerspectiveImageAnalyzer::placed_analyze()
{
    // 颜色转换
    cv::Mat hsv;
    cv::cvtColor(m_image, hsv, cv::COLOR_BGR2HSV);
    cv::Mat bin;
    cv::inRange(hsv, cv::Scalar(60, 100, 60), cv::Scalar(80, 150, 120), bin);

    // 形态学降噪
    cv::Mat morph_dst = bin;
    cv::Mat morph_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(9, 9));
    cv::morphologyEx(morph_dst, morph_dst, cv::MORPH_CLOSE, morph_kernel);
    cv::morphologyEx(morph_dst, morph_dst, cv::MORPH_OPEN, morph_kernel);

    const auto& all_homes = get_homes();
    if (all_homes.empty()) {
        return false;
    }
    Rect home = all_homes.front();

    int min_dist = INT_MAX;
    for (int row = 0; row != morph_dst.rows; ++row) {
        for (int col = 0; col != morph_dst.cols; ++col) {
            cv::uint8_t value = morph_dst.at<cv::uint8_t>(row, col);
            if (value) {
                int dist = std::abs(home.x - col) + std::abs(home.y - row);
                if (min_dist > dist) {
                    min_dist = dist;
                    m_nearest_point = Point(col, row);
                }
            }
        }
    }
    if (m_nearest_point.x == 0 && m_nearest_point.y == 0) {
        return false;
    }

#ifdef ASST_DEBUG
    cv::circle(m_image_draw, cv::Point(m_nearest_point.x, m_nearest_point.y), 3, cv::Scalar(255, 255, 0));
#endif

//    // 格子的边缘是没法上人的，往格子中间位置校正一下
//    Point home_center(home.x + home.width / 2, home.y + home.height / 2);
//    m_nearest_point.x += home_center.x < m_nearest_point.x ? 30 : -30;
//    m_nearest_point.y += home_center.y < m_nearest_point.y ? 20 : -20;
//
//#ifdef ASST_DEBUG
//    cv::circle(m_image_draw, cv::Point(m_nearest_point.x, m_nearest_point.y), 3, cv::Scalar(255, 255, 0), -1);
//#endif
    return true;
}
