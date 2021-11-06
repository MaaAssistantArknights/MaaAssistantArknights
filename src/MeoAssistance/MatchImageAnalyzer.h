/*
    MeoAssistance (CoreLib) - A part of the MeoAssistance-Arknight project
    Copyright (C) 2021 MistEO and Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst {
    class MatchImageAnalyzer : public AbstractImageAnalyzer {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        MatchImageAnalyzer(const cv::Mat& image, const Rect& roi, std::string templ_name, double templ_thres = 0.0, double hist_thres = 0.0);
        virtual ~MatchImageAnalyzer() = default;

        virtual bool analyze() override;
        void set_use_cache(bool is_use) noexcept {
            m_use_cache = is_use;
        }
        void set_mask_range(int lower, int upper) {
            m_mask_range = std::make_pair(lower, upper);
        }
        void set_mask_range(std::pair<int, int> mask_range) {
            m_mask_range = std::move(mask_range);
        }
        void set_templ_name(std::string templ_name) noexcept {
            m_templ_name = std::move(templ_name);
        }
        void set_threshold(double templ_thres, double hist_thres = 0.0) noexcept {
            m_templ_thres = templ_thres;
            m_hist_thres = hist_thres;
        }
        void set_task_info(MatchTaskInfo task_info) noexcept {
            m_use_cache = task_info.cache;
            m_mask_range = std::move(task_info.mask_range);
            m_templ_name = std::move(task_info.templ_name);
            m_templ_thres = task_info.templ_threshold;
            m_hist_thres = task_info.hist_threshold;
            set_roi(task_info.roi);
        }

        //const std::vector<MatchRect>& get_result() const noexcept {
        //    return m_result;
        //}
        const MatchRect& get_result() const noexcept {
            return m_result;
        }

    protected:
        static cv::Mat to_hist(const cv::Mat& src);

        virtual bool match_templ(const cv::Mat& templ);
        virtual bool comp_hist(const cv::Mat& hist, const cv::Rect roi);

        std::string m_templ_name;
        MatchRect m_result;
        double m_templ_thres = 0.0;
        double m_hist_thres = 0.0;
        bool m_use_cache = false;
        std::pair<int, int> m_mask_range;
    };
}
