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
            std::vector<MatchTaskInfo::Range> mask_range;
            std::vector<MatchMethod> methods;
            bool mask_with_src = false;
            bool mask_with_close = false;
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
        void set_mask_range(int lower, int upper, bool mask_with_src = false, bool mask_with_close = false);
        void set_mask_range(
            std::vector<MatchTaskInfo::Range> mask_range,
            bool mask_with_src = false,
            bool mask_with_close = false);
        void set_method(MatchMethod method) noexcept;

    protected:
        virtual void _set_roi(const Rect& roi) = 0;

        void _set_task_info(MatchTaskInfo task_info);

    protected:
        Params m_params;
    };
}
