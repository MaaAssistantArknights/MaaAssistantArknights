#include "StartUpEventPagePlugin.h"

#include <algorithm>
#include <array>

#include "Controller/Controller.h"
#include "Controller/Win32Controller.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"

bool asst::StartUpEventPagePlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string task = details.get("details", "task", "");
    return task == EscTaskName || task.ends_with("@" + std::string(EscTaskName));
}

bool asst::StartUpEventPagePlugin::_run()
{
    const cv::Mat image = ctrler()->get_image();
    if (!is_pc_event_page(image)) {
        Log.info(__FUNCTION__, "| PC event page icon pattern not detected, skip ESC fallback");
        return true;
    }

    Log.info(__FUNCTION__, "| PC event page icon pattern detected, press ESC");
#ifdef _WIN32
    if (auto* win32_controller = dynamic_cast<Win32Controller*>(ctrler()->get_underlying())) {
        return win32_controller->press_esc_with_foreground();
    }
#endif
    return ctrler()->press_esc();
}

bool asst::StartUpEventPagePlugin::is_pc_event_page(const cv::Mat& image)
{
    if (image.empty()) {
        return false;
    }

    const double scale = image.cols / 1280.0;
    const int roi_width = std::clamp(static_cast<int>(330 * scale), 1, image.cols);
    const int roi_height = std::clamp(static_cast<int>(95 * scale), 1, image.rows);
    const cv::Rect roi(0, 0, roi_width, roi_height);

    cv::Mat hsv;
    cv::cvtColor(image(roi), hsv, cv::COLOR_BGR2HSV);

    cv::Mat mask;
    cv::inRange(hsv, cv::Scalar(0, 0, 145), cv::Scalar(179, 85, 255), mask);
    const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);

    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int component_count = cv::connectedComponentsWithStats(mask, labels, stats, centroids, 8);

    std::vector<double> icon_centers;
    const int min_area = std::max(80, static_cast<int>(80 * scale * scale));
    const int max_area = std::max(min_area + 1, static_cast<int>(5000 * scale * scale));
    const int min_size = std::max(8, static_cast<int>(8 * scale));
    const int max_size = std::max(min_size + 1, static_cast<int>(70 * scale));
    const int max_top = static_cast<int>(70 * scale);

    for (int index = 1; index < component_count; ++index) {
        const int y = stats.at<int>(index, cv::CC_STAT_TOP);
        const int width = stats.at<int>(index, cv::CC_STAT_WIDTH);
        const int height = stats.at<int>(index, cv::CC_STAT_HEIGHT);
        const int area = stats.at<int>(index, cv::CC_STAT_AREA);

        if (area < min_area || area > max_area || width < min_size || width > max_size || height < min_size ||
            height > max_size || y > max_top) {
            continue;
        }

        icon_centers.emplace_back(centroids.at<double>(index, 0));
    }

    std::ranges::sort(icon_centers);

    std::vector<double> separated_centers;
    const double min_gap = 35 * scale;
    for (const double center : icon_centers) {
        if (separated_centers.empty() || center - separated_centers.back() >= min_gap) {
            separated_centers.emplace_back(center);
        }
    }

    Log.info(
        __FUNCTION__,
        "| detected top-left icon components:",
        separated_centers.size(),
        "image:",
        image.cols,
        "x",
        image.rows);

    return separated_centers.size() >= 3;
}
