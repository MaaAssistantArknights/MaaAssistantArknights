#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst
{
    class MultiMatchImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        MultiMatchImageAnalyzer(const cv::Mat& image, std::string templ_name, double templ_thres);
        virtual ~MultiMatchImageAnalyzer() override = default;

        virtual bool analyze() override;

        // 按位置排序
        // 1 - 2 - 3
        // 4 - 5 - 6
        void sort_result_horizontal();

        // 按位置排序
        // 1 - 3 - 5
        // 2 - 4 - 6
        void sort_result_vertical();

        void set_mask_range(int lower, int upper) noexcept;
        void set_mask_range(std::pair<int, int> mask_range) noexcept;
        void set_templ_name(std::string templ_name) noexcept;
        void set_threshold(double templ_thres) noexcept;

        void set_task_info(const std::shared_ptr<TaskInfo>& task_ptr);
        void set_task_info(const std::string& task_name);

        const std::vector<MatchRect>& get_result() const noexcept;

    protected:
        virtual bool multi_match_templ(const cv::Mat templ);
        void set_task_info(MatchTaskInfo task_info) noexcept;

        std::string m_templ_name;
        double m_templ_thres = 0.0;
        std::vector<MatchRect> m_result;
        std::pair<int, int> m_mask_range;
    };
}
