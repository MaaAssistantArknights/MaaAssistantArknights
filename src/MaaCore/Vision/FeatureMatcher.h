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
    // foreach_templ函数用于对template进行特征匹配
    std::vector<Result> foreach_templ(const cv::Mat& image) const;
    // create_detector函数用于创建特征检测器
    cv::Ptr<cv::Feature2D> create_detector() const;
    // detect函数用于检测图像中的特征点
    std::pair<std::vector<cv::KeyPoint>, cv::Mat> detect(const cv::Mat& image, const cv::Mat& mask) const;
    // create_matcher函数用于创建特征匹配器
    cv::Ptr<cv::DescriptorMatcher> create_matcher() const;
    // match函数用于匹配两组特征描述符
    std::vector<std::vector<cv::DMatch>>
        match(const cv::Mat& descriptors_templ, const cv::Mat& descriptors_image) const;
    // postproc函数用于对匹配结果进行后处理
    std::vector<Result> postproc(
        const cv::Mat& templ,
        const std::vector<std::vector<cv::DMatch>>& match_points,
        const std::vector<cv::KeyPoint>& keypoints_1,
        const std::vector<cv::KeyPoint>& keypoints_2,
        const cv::Rect& roi_2) const;
    // draw_result函数用于绘制匹配结果
    void draw_result(
        const cv::Mat& templ,
        const std::vector<cv::KeyPoint>& keypoints_1,
        const cv::Rect& roi,
        const std::vector<cv::KeyPoint>& keypoints_2,
        const std::vector<cv::DMatch>& good_matches,
        std::vector<Result>& results) const;
    // filter函数用于过滤匹配结果
    void filter(std::vector<Result>& results, int count) const;
};

} // namespace asst
