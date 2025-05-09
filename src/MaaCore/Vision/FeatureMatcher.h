#pragma once
#include "Vision/Config/FeatureMatcherConfig.h"
#include "VisionHelper.h"

#include <meojson/json.hpp>
#include <ostream>
#include <vector>

// MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include <opencv2/features2d.hpp>

// MAA_SUPPRESS_CV_WARNINGS_END

namespace asst
{
class FeatureMatcher : public VisionHelper, public FeatureMatcherConfig
{
public:
    using Result = FeatureMatchRect;
    using ResultsVec = std::vector<Result>;
    using ResultsVecOpt = std::optional<ResultsVec>;

public:
    using VisionHelper::VisionHelper;
    virtual ~FeatureMatcher() override = default;

    // analyze函数用于分析图像，返回匹配结果
    ResultsVecOpt analyze() const;

    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    const auto& get_result() const noexcept { return m_result; }

protected:
    virtual void _set_roi(const Rect& roi) override { set_roi(roi); }

private:
    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    mutable ResultsVec m_result;

private:
    std::pair<std::vector<cv::KeyPoint>, cv::Mat> detect(const cv::Mat& image, const cv::Mat& mask) const;
    asst::FeatureMatcher::ResultsVec feature_match(
        const cv::Mat& templ,
        const std::vector<cv::KeyPoint>& keypoints_1,
        const cv::Mat& descriptors_1) const;
    std::vector<std::vector<cv::DMatch>> match(const cv::Mat& descriptors_1, const cv::Mat& descriptors_2) const;
    asst::FeatureMatcher::ResultsVec feature_postproc(
        const std::vector<std::vector<cv::DMatch>>& match_points,
        const std::vector<cv::KeyPoint>& keypoints_1,
        const std::vector<cv::KeyPoint>& keypoints_2,
        int templ_cols,
        int templ_rows,
        std::vector<cv::DMatch>& good_matches) const;
    cv::Ptr<cv::Feature2D> create_detector() const;
    cv::Ptr<cv::DescriptorMatcher> create_matcher() const;
};

} // namespace asst
