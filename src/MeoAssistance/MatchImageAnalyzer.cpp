﻿/*
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

#include "MatchImageAnalyzer.h"

#include "AsstUtils.hpp"
#include "Logger.hpp"
#include "Resource.h"

asst::MatchImageAnalyzer::MatchImageAnalyzer(const cv::Mat& image, const Rect& roi, std::string templ_name, double templ_thres, double hist_thres)
    : AbstractImageAnalyzer(image, roi),
      m_templ_name(std::move(templ_name)),
      m_templ_thres(templ_thres),
      m_hist_thres(hist_thres) {
    ;
}

bool asst::MatchImageAnalyzer::analyze() {
    if (m_use_cache) {
        auto&& [hist, roi] = resource.templ().get_hist(m_templ_name);
        if (!hist.empty() && comp_hist(hist, roi)) {
            return true;
        }
    }
    const cv::Mat& templ = resource.templ().get_templ(m_templ_name);
    if (templ.empty()) {
        log.error("templ is empty!");
        return false;
    }
    if (match_templ(templ)) {
        if (m_use_cache) {
            resource.templ().emplace_hist(m_templ_name, to_hist(templ), utils::make_rect<cv::Rect>(m_result.rect));
        }
        return true;
    }
    return false;
}

cv::Mat asst::MatchImageAnalyzer::to_hist(const cv::Mat& src) {
    constexpr int histSize[] = { 50, 60 };
    constexpr float h_ranges[] = { 0, 180 };
    constexpr float s_ranges[] = { 0, 256 };
    const float* ranges[] = { h_ranges, s_ranges };
    constexpr int channels[] = { 0, 1 };

    cv::MatND hist;

    cv::calcHist(&src, 1, channels, cv::Mat(), hist, 2, histSize, ranges);
    cv::normalize(hist, hist, 0, 1, cv::NORM_MINMAX);

    return hist;
}

bool asst::MatchImageAnalyzer::match_templ(const cv::Mat& templ) {
    cv::Mat matched;

    cv::Mat image_roi = m_image(utils::make_rect<cv::Rect>(m_roi));
    if (m_mask_range.first == m_mask_range.second) {
        cv::matchTemplate(image_roi, templ, matched, cv::TM_CCOEFF_NORMED);
    }
    else {
        cv::Mat mask;
        cv::cvtColor(templ, mask, cv::COLOR_BGR2GRAY);
        cv::inRange(mask, m_mask_range.first, m_mask_range.second, mask);
        cv::matchTemplate(image_roi, templ, matched, cv::TM_CCOEFF_NORMED, mask);
    }
    double min_val = 0.0, max_val = 0.0;
    cv::Point min_loc, max_loc;
    cv::minMaxLoc(matched, &min_val, &max_val, &min_loc, &max_loc);

    if (max_val > m_templ_thres * 0.7) { // 得分太低的肯定不对，没必要打印
        log.trace("match_templ |", m_templ_name, "score:", max_val, "point:", max_loc);
    }

    if (max_val >= m_templ_thres) {
        Rect rect(max_loc.x + m_roi.x, max_loc.y + m_roi.y, templ.cols, templ.rows);
        m_result = { AlgorithmType::MatchTemplate, max_val, rect };
        return true;
    }
    else {
        return false;
    }
}

bool asst::MatchImageAnalyzer::comp_hist(const cv::Mat& hist, const cv::Rect roi) {
    cv::Mat image_roi = m_image(utils::make_rect<cv::Rect>(m_roi))(roi);
    double score = 1.0 - cv::compareHist(to_hist(image_roi), hist, cv::HISTCMP_BHATTACHARYYA);

    if (score > 0.7) { // 得分太低的肯定不对，没必要打印
        log.trace("comp_hist |", m_templ_name, "score:", score);
    }

    if (score >= m_hist_thres) {
        Rect rect(roi.x + m_roi.x, roi.y + m_roi.y, hist.cols, hist.rows);
        m_result = { AlgorithmType::CompareHist, score, rect };
        return true;
    }
    else {
        return false;
    }
}