#pragma once
#include "Common/AsstTypes.h"
#include "Utils/NoWarningCVMat.h"
#include <variant>

namespace asst
{
class FeatureMatcherConfig
{
public:
    using Detector = FeatureDetector;

    // enum class Matcher
    //{
    //     FLANN,
    //     BRUTEFORCE,
    // };

    struct Params
    {
        std::variant<std::string, cv::Mat> templs;
        bool green_mask = false;

        Detector detector = Detector::SIFT;
        // Matcher matcher = Matcher::FLANN;

        double distance_ratio = 0.6; // KNN 匹配算法的距离比值，[0 - 1.0], 越大则匹配越宽松,更容易连线。默认0.6
        int count = 4;               // 匹配的特征点的数量要求（阈值），默认4
    };

public:
    FeatureMatcherConfig() = default;
    virtual ~FeatureMatcherConfig() = default;

    void set_task_info(const std::shared_ptr<TaskInfo>& task_ptr);
    void set_task_info(const std::string& task_name);

    void set_params(Params params) { m_params = std::move(params); }

    void set_templ(std::variant<std::string, cv::Mat> templ) { m_params.templs = { std::move(templ) }; }

    void set_detector(Detector detector) noexcept { m_params.detector = detector; }

    void set_distance_ratio(double distance_ratio) noexcept { m_params.distance_ratio = distance_ratio; }

    void set_count(int count) noexcept { m_params.count = count; }

protected:
    virtual void _set_roi(const Rect& roi) = 0;
    virtual void _set_task_info(FeatureMatchTaskInfo task_info);

    // void _set_task_info(MatchTaskInfo task_info);

protected:
    Params m_params;
};
}
