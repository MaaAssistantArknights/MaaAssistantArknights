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
        void set_mask_range(int lower, int upper)
        {
            m_mask_range = std::make_pair(lower, upper);
        }
        void set_mask_range(std::pair<int, int> mask_range)
        {
            m_mask_range = std::move(mask_range);
        }
        void set_templ_name(std::string templ_name) noexcept
        {
            m_templ_name = std::move(templ_name);
        }
        void set_threshold(double templ_thres) noexcept
        {
            m_templ_thres = templ_thres;
        }
        void set_task_info(MatchTaskInfo task_info) noexcept
        {
            m_mask_range = std::move(task_info.mask_range);
            m_templ_name = std::move(task_info.templ_name);
            m_templ_thres = task_info.templ_threshold;
            set_roi(task_info.roi);
        }
        const std::vector<MatchRect>& get_result() const noexcept
        {
            return m_result;
        }

    protected:
        virtual bool multi_match_templ(const cv::Mat templ);

        std::string m_templ_name;
        double m_templ_thres = 0.0;
        std::vector<MatchRect> m_result;
        std::pair<int, int> m_mask_range;
    };
}
