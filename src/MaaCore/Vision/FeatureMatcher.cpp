#include "FeatureMatcher.h"

#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"

// MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include <opencv2/features2d.hpp>
#include <opencv2/opencv.hpp>
#ifdef MAA_VISION_HAS_XFEATURES2D
#include <opencv2/xfeatures2d.hpp>
#endif
// MAA_SUPPRESS_CV_WARNINGS_END

asst::FeatureMatcher::ResultsVecOpt asst::FeatureMatcher::analyze() const
{
    auto start_time = std::chrono::steady_clock::now();
    const auto& templ_ptr = m_params.templs;
    cv::Mat templ;
    std::string templ_name;

    if (std::holds_alternative<std::string>(templ_ptr)) {
        templ_name = std::get<std::string>(templ_ptr);
        templ = TemplResource::get_instance().get_templ(templ_name);
    }
    else if (std::holds_alternative<cv::Mat>(templ_ptr)) {
        templ = std::get<cv::Mat>(templ_ptr);
    }
    else {
        Log.error("templ is none");
    }

    if (templ.empty()) {
        Log.error("templ is empty!", templ_name);
#ifdef ASST_DEBUG
        throw std::runtime_error("templ is empty: " + templ_name);
#else
        return std::nullopt;
#endif
    }

    if (templ.cols > m_image.cols || templ.rows > m_image.rows) {
        LogError << "templ size is too large" << templ_name << "image size:" << m_image.cols << m_image.rows
                 << "templ size:" << templ.cols << templ.rows;
        return std::nullopt;
    }

    auto [keypoints_1, descriptors_1] = detect(templ, create_mask(templ, m_params.green_mask));

    auto results = feature_match(templ, keypoints_1, descriptors_1);
#ifdef ASST_DEBUG
    const auto& color = cv::Scalar(0, 0, 255);
#endif
    for (const auto& r : results) {
        if (r.count < m_params.count) {
            Log.debug("feature_match |", templ_name, "count:", r.count, "rect:", r.rect, "roi:", m_roi);
        }
        else {
            Log.trace("feature_match |", templ_name, "count:", r.count, "rect:", r.rect, "roi:", m_roi);
        }
#ifdef ASST_DEBUG
        cv::putText(
            m_image_draw,
            "count: " + std::to_string(r.count),
            cv::Point(r.rect.x, r.rect.y - 5),
            cv::FONT_HERSHEY_PLAIN,
            1.2,
            color,
            1);
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(r.rect), cv::Scalar(0, 0, 255), 2);
#endif
    }
    std::erase_if(results, [&](const auto& res) { return res.count < m_params.count; });
    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
    if (results.empty()) {
        return std::nullopt;
    }

    Log.trace("count:", results.size(), ", cost:", cost.count(), "ms");
    m_result = std::move(results);
    return m_result;
}

std::pair<std::vector<cv::KeyPoint>, cv::Mat>
    asst::FeatureMatcher::detect(const cv::Mat& image, const cv::Mat& mask) const
{
    auto detector = create_detector();
    if (!detector) {
        LogError << "detector is empty";
        return {};
    }

    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    detector->detectAndCompute(image, mask, keypoints, descriptors);

    return std::make_pair(std::move(keypoints), std::move(descriptors));
}

asst::FeatureMatcher::ResultsVec asst::FeatureMatcher::feature_match(
    const cv::Mat& templ,
    const std::vector<cv::KeyPoint>& keypoints_1,
    const cv::Mat& descriptors_1) const
{
    auto [keypoints_2, descriptors_2] = detect(m_image, create_mask(m_image, make_rect<cv::Rect>(m_roi)));

    auto match_points = match(descriptors_1, descriptors_2);

    std::vector<cv::DMatch> good_matches;
    ResultsVec results = feature_postproc(match_points, keypoints_1, keypoints_2, templ.cols, templ.rows, good_matches);

#ifdef ASST_DEBUG
    cv::Mat matches_draw;
    cv::drawMatches(m_image_draw, keypoints_2, templ, keypoints_1, good_matches, matches_draw);
#endif // ASST_DEBUG
    return results;
}

std::vector<std::vector<cv::DMatch>>
    asst::FeatureMatcher::match(const cv::Mat& descriptors_1, const cv::Mat& descriptors_2) const
{
    if (descriptors_1.empty() || descriptors_2.empty()) {
        LogWarn << "descriptors is empty";
        return {};
    }

    auto matcher = create_matcher();
    if (!matcher) {
        LogError << "matcher is empty";
        return {};
    }

    std::vector<cv::Mat> train_desc(1, descriptors_1);
    matcher->add(train_desc);
    matcher->train();

    std::vector<std::vector<cv::DMatch>> match_points;
    matcher->knnMatch(descriptors_2, match_points, 2);
    return match_points;
}

asst::FeatureMatcher::ResultsVec asst::FeatureMatcher::feature_postproc(
    const std::vector<std::vector<cv::DMatch>>& match_points,
    const std::vector<cv::KeyPoint>& keypoints_1,
    const std::vector<cv::KeyPoint>& keypoints_2,
    int templ_cols,
    int templ_rows,
    std::vector<cv::DMatch>& good_matches) const
{
    std::vector<cv::Point2d> obj;
    std::vector<cv::Point2d> scene;

    for (const auto& point : match_points) {
        if (point.size() != 2) {
            continue;
        }

        double threshold = m_params.distance_ratio * point[1].distance;
        if (point[0].distance > threshold) {
            continue;
        }
        good_matches.emplace_back(point[0]);
        obj.emplace_back(keypoints_1[point[0].trainIdx].pt);
        scene.emplace_back(keypoints_2[point[0].queryIdx].pt);
    }
    LogDebug << "Match:" << VAR(good_matches.size()) << VAR(match_points.size()) << VAR(m_params.distance_ratio);

    const std::array<cv::Point2d, 4> obj_corners = {
        cv::Point2d(0, 0),
        cv::Point2d(templ_cols, 0),
        cv::Point2d(templ_cols, templ_rows),
        cv::Point2d(0, templ_rows),
    };

    ResultsVec results;
    while (scene.size() >= 4) {
        cv::Mat homography = cv::findHomography(obj, scene, cv::RANSAC);
        if (homography.empty()) {
            break;
        }

        std::array<cv::Point2d, 4> scene_corners;
        cv::perspectiveTransform(obj_corners, scene_corners, homography);

        double x = std::min({ scene_corners[0].x, scene_corners[1].x, scene_corners[2].x, scene_corners[3].x });
        double y = std::min({ scene_corners[0].y, scene_corners[1].y, scene_corners[2].y, scene_corners[3].y });
        double w = std::max({ scene_corners[0].x, scene_corners[1].x, scene_corners[2].x, scene_corners[3].x }) - x;
        double h = std::max({ scene_corners[0].y, scene_corners[1].y, scene_corners[2].y, scene_corners[3].y }) - y;
        cv::Rect scene_box { static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h) };

        cv::Rect box = scene_box & make_rect<cv::Rect>(m_roi);
        size_t count = std::ranges::count_if(scene, [&box](const auto& point) { return box.contains(point); });
        if (count == 0) {
            LogTrace << "No points in box" << VAR(box) << VAR(scene_box) << VAR(m_roi);
            break;
        }

        results.emplace_back(Result { .rect = make_rect<asst::Rect>(box), .count = static_cast<int>(count) });

        // remove inside points
        size_t compact_idx = 0;
        for (size_t i = 0; i < scene.size(); ++i) {
            if (scene_box.contains(scene.at(i))) {
                continue;
            }

            if (i != compact_idx) {
                std::swap(scene[compact_idx], scene[i]);
                std::swap(obj[compact_idx], obj[i]);
            }
            ++compact_idx;
        }
        scene.resize(compact_idx);
        obj.resize(compact_idx);
    }

    return results;
}

cv::Ptr<cv::Feature2D> asst::FeatureMatcher::create_detector() const
{
    switch (m_params.detector) {
    case FeatureMatcherConfig::Detector::SIFT:
        return cv::SIFT::create();
    case FeatureMatcherConfig::Detector::ORB:
        return cv::ORB::create();
    case FeatureMatcherConfig::Detector::BRISK:
        return cv::BRISK::create();
    case FeatureMatcherConfig::Detector::KAZE:
        return cv::KAZE::create();
    case FeatureMatcherConfig::Detector::AKAZE:
        return cv::AKAZE::create();
    case FeatureMatcherConfig::Detector::SURF:
#ifdef MAA_VISION_HAS_XFEATURES2D
        return cv::xfeatures2d::SURF::create();
#else
        Log.error("SURF not enabled!");
        return nullptr;
#endif
    }

    Log.error("Unknown detector", static_cast<int>(m_params.detector));
    return nullptr;
}

cv::Ptr<cv::DescriptorMatcher> asst::FeatureMatcher::create_matcher() const
{
    switch (m_params.detector) {
    case FeatureMatcherConfig::Detector::SIFT:
    case FeatureMatcherConfig::Detector::SURF:
    case FeatureMatcherConfig::Detector::KAZE:
        return cv::FlannBasedMatcher::create();

    case FeatureMatcherConfig::Detector::ORB:
    case FeatureMatcherConfig::Detector::BRISK:
    case FeatureMatcherConfig::Detector::AKAZE:
        return cv::BFMatcher::create(cv::NORM_HAMMING);
    }

    Log.error("Unknown detector", static_cast<int>(m_params.detector));
    return nullptr;
}
