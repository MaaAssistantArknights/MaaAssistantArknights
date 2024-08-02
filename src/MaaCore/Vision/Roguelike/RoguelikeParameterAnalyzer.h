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
    virtual ~RoguelikeParameterAnalyzer() noexcept override = default;

    //bool analyze();

    int get_number(const cv::Mat& image, const std::string& task_name);

    // int update_hope(const cv::Mat& image);
    int update_hp(const cv::Mat& image);
    // int update_formation_upper_limit(const cv::Mat& image);
    
    //  识别萨米肉鸽抗干扰
    int update_chaos(const cv::Mat& image);

    // 识别萨卡兹肉鸽构想数量
    int update_idea_count(const cv::Mat& image);
    // 识别萨卡兹肉鸽当前负荷
    // int update_burden_number(const cv::Mat& image);
    // 识别萨卡兹肉鸽负荷上限
    // int update_burden_upper_limit(const cv::Mat& image);
private:
    InstHelper m_inst_helper;
};
}
