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
    cv::inRange(hsv, cv::Scalar(55, 90, 70), cv::Scalar(80, 150, 150), bin);

    // 形态学降噪
    cv::Mat morph_dst = bin;
    cv::Mat morph_open_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(9, 9));
    cv::morphologyEx(morph_dst, morph_dst, cv::MORPH_OPEN, morph_open_kernel);
    //cv::Mat morph_close_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    //cv::morphologyEx(morph_dst, morph_dst, cv::MORPH_CLOSE, morph_close_kernel);

    // 计算可放干员格子的连通域，圈出每个单独的格子
    cv::Mat out, stats, centroids;
    int number = cv::connectedComponentsWithStats(morph_dst, out, stats, centroids);
    if (number < 2) {
        return false;
    }
    std::vector<Point> placed_centers;
    for (int i = 1; i != number; ++i) { // 第 0 个是整张图，所以从 1 开始
        int area = stats.at<int>(i, cv::CC_STAT_AREA);
        if (area < 300) {
            continue;
        }
        int x = stats.at<int>(i, cv::CC_STAT_LEFT);
        int y = stats.at<int>(i, cv::CC_STAT_TOP);
        int w = stats.at<int>(i, cv::CC_STAT_WIDTH);
        int h = stats.at<int>(i, cv::CC_STAT_HEIGHT);
        m_available_placed.emplace_back(Rect(x, y, w, h));

        int center_x = static_cast<int>(centroids.at<double>(i, 0));
        int center_y = static_cast<int>(centroids.at<double>(i, 1));
        placed_centers.emplace_back(Point(center_x, center_y));

#ifdef ASST_DEBUG
        cv::circle(m_image_draw, cv::Point(center_x, center_y), 3, cv::Scalar(0, 0, 255), -1);
        cv::rectangle(m_image_draw, cv::Rect(x, y, w, h), cv::Scalar(255, 0, 0), 3);
#endif
    }
    if (m_available_placed.empty()) {
        return false;
    }

    const auto& all_homes = get_homes();
    if (all_homes.empty()) {
        return false;
    }
    Rect home = all_homes.front();

    auto nearest_iter = std::min_element(placed_centers.cbegin(), placed_centers.cend(),
        [&home](const Point& lhs, const Point& rhs) -> bool {
            double lhs_dist = std::pow((home.x - lhs.x), 2) + std::pow((home.y - lhs.y), 2);
            double rhs_dist = std::pow((home.x - rhs.x), 2) + std::pow((home.y - rhs.y), 2);
            return lhs_dist < rhs_dist;
    });
    if (nearest_iter == placed_centers.cend()) {
        return false;
    }
    m_nearest_point = *nearest_iter;

    return true;
}
