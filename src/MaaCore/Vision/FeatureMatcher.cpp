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
    // name_ 为task.name

    auto start_time = std::chrono::steady_clock::now();

    std::vector<Result> results = foreach_templ(m_image);

    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
    Log.trace("Raw:", results, ",cost:", cost);

    int count = m_params.count;
    filter(results, count);

    cost = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
    Log.trace("Filter:", results, ",count:", count, ",cost:", cost);

    if (results.empty()) {
        return std::nullopt;
    }
    for (const auto& r : results) {
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(r.rect), cv::Scalar(0, 0, 255), 2);
#endif
    }
    m_result = std::move(results);
    return m_result;
}

std::vector<asst::FeatureMatcher::Result> asst::FeatureMatcher::foreach_templ(const cv::Mat& image) const
{
    std::vector<Result> results;
    for (const auto& ptempl : m_params.templs) {
        cv::Mat templ;
        std::string templ_name;

        if (std::holds_alternative<std::string>(ptempl)) {
            templ_name = std::get<std::string>(ptempl);
            templ = TemplResource::get_instance().get_templ(templ_name);
        }
        else if (std::holds_alternative<cv::Mat>(ptempl)) {
            templ = std::get<cv::Mat>(ptempl);
        }
        else {
            Log.error("templ is none");
        }

        if (templ.empty()) {
            Log.error("templ is empty!", templ_name);
#ifdef ASST_DEBUG
            throw std::runtime_error("templ is empty: " + templ_name);
#else
            return {};
#endif
        }

        if (templ.cols > image.cols || templ.rows > image.rows) {
            Log.error(
                "templ size is too large",
                templ_name,
                "image size:",
                image.cols,
                image.rows,
                "templ size:",
                templ.cols,
                templ.rows);
            return {};
        }

        auto [keypoints_templ, descriptors_templ] = detect(templ, create_mask(templ, m_params.green_mask));
        auto [keypoints_image, descriptors_image] = detect(m_image, create_mask(m_image, make_rect<cv::Rect>(m_roi)));

        auto match_points = match(descriptors_templ, descriptors_image);
        auto res = postproc(templ, match_points, keypoints_templ, keypoints_image, make_rect<cv::Rect>(m_roi));

        results.insert(results.end(), std::make_move_iterator(res.begin()), std::make_move_iterator(res.end()));
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

std::pair<std::vector<cv::KeyPoint>, cv::Mat>
    asst::FeatureMatcher::detect(const cv::Mat& image, const cv::Mat& mask) const
{
    auto detector = create_detector();
    if (!detector) {
        Log.error("detector is empty");
        return {};
    }

    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    detector->detectAndCompute(image, mask, keypoints, descriptors);

    return std::make_pair(std::move(keypoints), std::move(descriptors));
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

std::vector<std::vector<cv::DMatch>>
    asst::FeatureMatcher::match(const cv::Mat& descriptors_templ, const cv::Mat& descriptors_image) const
{
    if (descriptors_templ.empty() || descriptors_image.empty()) {
        Log.error("descriptors is empty");
        return {};
    }

    auto matcher = create_matcher();
    if (!matcher) {
        Log.error("matcher is empty");
        return {};
    }

    std::vector<cv::Mat> train_desc(1, descriptors_templ);
    matcher->add(train_desc);
    matcher->train();

    std::vector<std::vector<cv::DMatch>> match_points;
    matcher->knnMatch(descriptors_image, match_points, 2);
    return match_points;
}

std::vector<asst::FeatureMatcher::Result> asst::FeatureMatcher::postproc(
    const cv::Mat& templ,
    const std::vector<std::vector<cv::DMatch>>& match_points,
    const std::vector<cv::KeyPoint>& keypoints_1,
    const std::vector<cv::KeyPoint>& keypoints_2,
    const cv::Rect& roi_2) const
{
    std::vector<cv::DMatch> good_matches;
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

    Log.trace(
        "Match:",
        good_matches.size(),
        ",match_points.size:",
        match_points.size(),
        ",m_params.distance_ratio:",
        m_params.distance_ratio);

    std::vector<Result> results;
    if (good_matches.size() < 4) {
        return results;
    }
    cv::Mat H = cv::findHomography(obj, scene, cv::RHO);

    std::array<cv::Point2d, 4> obj_corners = { cv::Point2d(0, 0),
                                               cv::Point2d(templ.cols, 0),
                                               cv::Point2d(templ.cols, templ.rows),
                                               cv::Point2d(0, templ.rows) };
    std::array<cv::Point2d, 4> scene_corners;
    if (!H.empty()) {
        cv::perspectiveTransform(obj_corners, scene_corners, H);

        double x = std::min({ scene_corners[0].x, scene_corners[1].x, scene_corners[2].x, scene_corners[3].x });
        double y = std::min({ scene_corners[0].y, scene_corners[1].y, scene_corners[2].y, scene_corners[3].y });
        double w = std::max({ scene_corners[0].x, scene_corners[1].x, scene_corners[2].x, scene_corners[3].x }) - x;
        double h = std::max({ scene_corners[0].y, scene_corners[1].y, scene_corners[2].y, scene_corners[3].y }) - y;
        cv::Rect box { static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h) };

        size_t count = ranges::count_if(scene, [&box](const auto& point) { return box.contains(point); });

        results.emplace_back(Result { .rect = make_rect<asst::Rect>(box), .count = static_cast<int>(count) });
    }

#ifdef ASST_DEBUG
    draw_result(templ, keypoints_1, roi_2, keypoints_2, good_matches, results);
#endif // ASST_DEBUG

    return results;
}

void asst::FeatureMatcher::draw_result(
    const cv::Mat& templ,
    const std::vector<cv::KeyPoint>& keypoints_1,
    const cv::Rect& roi,
    const std::vector<cv::KeyPoint>& keypoints_2,
    const std::vector<cv::DMatch>& good_matches,
    std::vector<Result>& results) const
{
    // const auto color = cv::Scalar(0, 0, 255);
    cv::Mat matches_draw;
    cv::drawMatches(m_image, keypoints_2, templ, keypoints_1, good_matches, matches_draw);

    cv::Mat image_draw = draw_roi(roi, matches_draw);
    const auto color = cv::Scalar(0, 0, 255);

    for (size_t i = 0; i != results.size(); ++i) {
        const auto& res = results.at(i);
        cv::rectangle(matches_draw, make_rect<cv::Rect>(res.rect), color, 1);
        /*
        std::string flag = MAA_FMT::format("Cnt: {}, [{}, {}, {}, {}]", res.count, res.box.x, res.box.y, res.box.width,
                                           res.box.height);*/
        std::string flag = "Cnt: " + std::to_string(res.count) + ", [" + std::to_string(res.rect.x) + ", " +
                           std::to_string(res.rect.y) + ", " + std::to_string(res.rect.width) + ", " +
                           std::to_string(res.rect.height) + "]";
        cv::putText(matches_draw, flag, cv::Point(res.rect.x, res.rect.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
    }

    /*
    if (save_draw_) {
        save_image(draw);
    }
    if (show_draw_) {
        const std::string kWinName = "Draw";
        cv::imshow(kWinName, draw);
        cv::waitKey(0);
        cv::destroyWindow(kWinName);
    }*/

    const std::string kWinName = "Draw";
    cv::imshow(kWinName, matches_draw);
    cv::waitKey(0);
    cv::destroyWindow(kWinName);
}

void asst::FeatureMatcher::filter(std::vector<Result>& results, int count) const
{
    auto remove_iter =
        std::remove_if(results.begin(), results.end(), [count](const auto& res) { return res.count < count; });
    results.erase(remove_iter, results.end());
}
