#pragma once

#include "Common/AsstTypes.h"
#include "Utils/NoWarningCVMat.h"

#include <variant>

namespace asst
{
class MatcherConfig
{
public:
    struct Params
    {
        std::vector<std::variant<std::string, cv::Mat>> templs;
        std::vector<double> templ_thres;
        std::vector<MatchMethod> methods;   // 匹配方法
        MatchTaskInfo::Ranges mask_ranges;  // 匹配时的颜色掩码范围
        bool mask_src = false;              // 匹配时是否使用原图掩码（默认使用模板掩码）
        bool mask_close = false;            // 匹配时是否使用闭运算处理
        MatchTaskInfo::Ranges color_scales; // 数色时的颜色掩码范围
        bool color_close = true;            // 数色时是否使用闭运算处理
    };

public:
    MatcherConfig() = default;
    virtual ~MatcherConfig() = default;

    void set_params(Params params);

    void set_task_info(const std::shared_ptr<TaskInfo>& task_ptr);
    void set_task_info(const std::string& task_name);

    void set_templ(std::variant<std::string, cv::Mat> templ);
    // void set_templ(std::vector<std::variant<std::string, cv::Mat>> templs);
    void set_threshold(double templ_thres) noexcept;
    void set_threshold(std::vector<double> templ_thres) noexcept;
    void set_mask_range(int lower, int upper, bool mask_src = false, bool mask_close = false);
    void set_mask_ranges(MatchTaskInfo::Ranges mask_ranges, bool mask_src = false, bool mask_close = false);
    void set_color_scales(MatchTaskInfo::Ranges color_scales, bool color_close = true);
    void set_method(MatchMethod method) noexcept;

protected:
    virtual void _set_roi(const Rect& roi) = 0;

    void _set_task_info(MatchTaskInfo task_info);

protected:
    Params m_params;
};
}
