#include "OperFilesImageAnalyzer.h"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Vision/BestMatcher.h"

namespace asst
{
namespace
{
// 圆点判白阈值（HSV V 通道）：灰点约 60-90，白点约 240+，取中间留足间隔。
constexpr int kDotWhiteVThreshold = 200;
// 圆心采样邻域半径：圆点直径约 6px，3x3 均值抑制压缩噪点与 1-2px 定位偏差。
constexpr int kDotSampleRadius = 1;
// 圆点连通域最小面积，过滤模板暗角噪点。
constexpr int kDotMinArea = 5;

// 圆点在图标内的相对位置从 3 级模板（三点全亮）提取，模板更新后无需改代码。
std::vector<cv::Point> mastery_dot_centers()
{
    const cv::Mat templ = TemplResource::get_instance().get_templ("OperFilesSkillMaster3.png");
    cv::Mat hsv, mask;
    cv::cvtColor(templ, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(0, 0, kDotWhiteVThreshold), cv::Scalar(180, 60, 255), mask);

    std::vector<cv::Point> centers;
    cv::Mat labels, stats, centroids;
    const int count = cv::connectedComponentsWithStats(mask, labels, stats, centroids, 8, CV_32S);
    for (int i = 1; i < count; ++i) { // 0 号连通域是背景
        if (stats.at<int>(i, cv::CC_STAT_AREA) < kDotMinArea) {
            continue;
        }
        centers.emplace_back(
            static_cast<int>(centroids.at<double>(i, 0)),
            static_cast<int>(centroids.at<double>(i, 1)));
    }
    return centers;
}
}

std::optional<int> asst::OperFilesImageAnalyzer::mastery_level(int skill)
{
    LogTraceFunction;

    if (skill < 1 || skill > 3) {
        Log.error(__FUNCTION__, "| invalid skill index", skill);
        return std::nullopt;
    }

    // 模板匹配只用于在任务 roi 内定位图标：0 级（全灰）与 3 级（全白）图标仅亮度不同，
    // TM_CCOEFF_NORMED 对亮度不敏感，二者得分几乎相同，不能按最高分模板判级；
    // 真实等级由点亮（白色）圆点数决定。
    const std::string task_name = "AutoRaise@CurrentSkill" + std::to_string(skill) + "MasterLevel";
    BestMatcher locator(m_image);
    locator.set_task_info(task_name);
    const auto locate_opt = locator.analyze();
    if (!locate_opt) {
        Log.warn(__FUNCTION__, "| mastery icon not found, task", task_name);
        return std::nullopt;
    }

    const std::vector<cv::Point> dot_centers = mastery_dot_centers();
    if (dot_centers.size() != 3) {
        Log.error(__FUNCTION__, "| unexpected dot count in OperFilesSkillMaster3.png:", dot_centers.size());
        return std::nullopt;
    }

    const cv::Mat icon = make_roi(m_image, locate_opt->rect);
    cv::Mat hsv;
    cv::cvtColor(icon, hsv, cv::COLOR_BGR2HSV);

    int lit_dots = 0;
    for (const auto& center : dot_centers) {
        const cv::Rect sample(
            center.x - kDotSampleRadius,
            center.y - kDotSampleRadius,
            kDotSampleRadius * 2 + 1,
            kDotSampleRadius * 2 + 1);
        if (sample.x < 0 || sample.y < 0 || sample.br().x > icon.cols || sample.br().y > icon.rows) {
            continue;
        }
        if (cv::mean(hsv(sample))[2] > kDotWhiteVThreshold) {
            ++lit_dots;
        }
    }
    return lit_dots;
}
} // namespace asst
