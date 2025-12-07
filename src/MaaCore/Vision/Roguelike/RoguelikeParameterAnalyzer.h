#pragma once
#include "Vision/VisionHelper.h"

#include "Config/TaskData.h"
#include "InstHelper.h"
#include "Vision/OCRer.h"

namespace asst
{
class RoguelikeParameterAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    RoguelikeParameterAnalyzer() = default;

    virtual ~RoguelikeParameterAnalyzer() noexcept override = default;

    // bool analyze();

    static int get_number(const cv::Mat& image, const std::string& task_name);

    // 不随主题变化的参数识别
    static int update_hp(const cv::Mat& image);

    // 随主题变化且每个主题都有的参数识别
    static int update_hope(const cv::Mat& image, const std::string& theme);
    static int update_ingot(const cv::Mat& image, const std::string& theme);
    // int update_formation_upper_limit(const cv::Mat& image, const std::string& theme);

    //  识别萨米肉鸽抗干扰
    static int update_chaos(const cv::Mat& image, const std::string& theme = "Sami");

    // 识别萨卡兹肉鸽构想数量
    static int update_idea_count(const cv::Mat& image, const std::string& theme = "Sarkaz");
    // 识别萨卡兹肉鸽当前负荷
    // int update_burden_number(const cv::Mat& image,const std::string& theme = "Sarkaz");
    // 识别萨卡兹肉鸽负荷上限
    // int update_burden_upper_limit(const cv::Mat& image,const std::string& theme = "Sarkaz");

    // 识别界园肉鸽票券数量
    static int update_ticket_count(const cv::Mat& image, const std::string& theme);
};
}
