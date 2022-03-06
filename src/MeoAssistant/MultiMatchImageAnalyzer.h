#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst
{
    class MultiMatchImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        MultiMatchImageAnalyzer(const cv::Mat image, const Rect& roi, std::string templ_name, double templ_thres);
        virtual ~MultiMatchImageAnalyzer() = default;

        virtual bool analyze() override;

        void sort_result(); // 按位置排序，左上角的排在前面

        void set_mask_range(int lower, int upper) noexcept;
        void set_mask_range(std::pair<int, int> mask_range) noexcept;
        void set_templ_name(std::string templ_name) noexcept;
        void set_threshold(double templ_thres) noexcept;

        void set_task_info(std::shared_ptr<TaskInfo> task_ptr);

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
