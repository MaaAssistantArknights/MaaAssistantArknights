#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst
{
    class MatchImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        MatchImageAnalyzer(const cv::Mat image, const Rect& roi, std::string templ_name, double templ_thres = 0.0, double hist_thres = 0.0);
        virtual ~MatchImageAnalyzer() = default;

        virtual bool analyze() override;
        void set_use_cache(bool is_use) noexcept
        {
            m_use_cache = is_use;
        }
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

            if (task_info.cache && !task_info.region_of_appeared.empty()) {
                m_roi = task_info.region_of_appeared;
            }
            else {
                set_roi(task_info.roi);
                correct_roi();
            }
        }

        //const std::vector<MatchRect>& get_result() const noexcept {
        //    return m_result;
        //}
        const MatchRect& get_result() const noexcept
        {
            return m_result;
        }

    protected:
        virtual bool match_templ(const cv::Mat templ);

        std::string m_templ_name;
        MatchRect m_result;
        double m_templ_thres = 0.0;
        bool m_use_cache = false;
        std::pair<int, int> m_mask_range;
    };
}
