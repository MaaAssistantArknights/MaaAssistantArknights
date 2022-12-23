#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst
{
    class MatchImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        MatchImageAnalyzer(const cv::Mat& image, std::string templ_name, double templ_thres = 0.0);
        virtual ~MatchImageAnalyzer() override = default;

        virtual bool analyze() override;

        void set_use_cache(bool is_use) noexcept;
        void set_mask_range(int lower, int upper, bool mask_with_src = false) noexcept;
        void set_mask_range(std::pair<int, int> mask_range, bool mask_with_src = false) noexcept;
        void set_templ_name(std::string templ_name) noexcept;
        void set_templ(cv::Mat templ) noexcept;
        void set_threshold(double templ_thres) noexcept;
        void set_task_info(const std::shared_ptr<TaskInfo>& task_ptr);
        void set_task_info(const std::string& task_name);
        void set_region_of_appeared(Rect region) noexcept;
        void set_mask_with_close(int with_close) noexcept;

        const MatchRect& get_result() const noexcept;

    protected:
        virtual bool match_templ(const cv::Mat templ);
        void set_task_info(MatchTaskInfo task_info) noexcept;

        std::string m_templ_name;
        cv::Mat m_templ;
        MatchRect m_result;
        double m_templ_thres = 0.0;
        bool m_use_cache = false;
        Rect m_region_of_appeared;
        std::pair<int, int> m_mask_range;
        bool m_mask_with_src = false;
        bool m_mask_with_close = false;
    };
}
