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

#include "InfrastFacilityImageAnalyzer.h"

#include "AsstUtils.hpp"
#include "Logger.hpp"
#include "MultiMatchImageAnalyzer.h"
#include "Resource.h"

bool asst::InfrastFacilityImageAnalyzer::analyze() {
    const static std::unordered_map<std::string, std::string>
        facility_task_name = {
            { "Dorm", "InfrastDorm" },
            { "ControlCenter", "InfrastControlCenter" },
            { "Mfg", "InfrastMfg" },
            { "Trade", "InfrastTrade" },
            { "Power", "InfrastPower" },
            { "Office", "InfrastOffice" },
            { "Reception", "InfrastReception" }
        };
    // 因为基建的缩放是不确定的，有可能是正常大小，也可能是最小化的
    // 所以对每种情况都进行一下识别，取其中得分最高的
    const static std::vector<std::string> task_name_suffix = { "", "Mini" };

    MultiMatchImageAnalyzer mm_analyzer(m_image);

    auto task_analyze = [&](const std::string& task_name) -> bool {
        const auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
            resource.task().task_ptr(task_name));
        mm_analyzer.set_task_info(*task_ptr);

        return mm_analyzer.analyze();
    };

    int cor_suffix_index = -1;

    for (const auto& key : m_to_be_analyzed) {
        auto find_iter = facility_task_name.find(key);
        if (find_iter == facility_task_name.cend()) {
            log.error("facility name error", key);
            continue;
        }
        std::string task_name = find_iter->second;
        std::vector<MatchRect> cur_facility_result;
        // 已知基建缩放状态的时候，只识别这个缩放状态下的就行了
        // 否则识别所有状态，直到找出正确的当前缩放状态
        if (cor_suffix_index >= 0) {
            if (!task_analyze(task_name + task_name_suffix.at(cor_suffix_index))) {
                continue;
            }
            mm_analyzer.sort_result();
            cur_facility_result = mm_analyzer.get_result();
        }
        else {
            double max_score = 0;
            for (size_t i = 0; i != task_name_suffix.size(); ++i) {
                if (!task_analyze(task_name + task_name_suffix.at(i))) {
                    continue;
                }

                const auto& cur_res = mm_analyzer.get_result();
                auto cur_max_iter = std::max_element(cur_res.cbegin(), cur_res.cend(),
                                                     [](const MatchRect& lhs, const MatchRect& rhs) -> bool {
                                                         return lhs.score < rhs.score;
                                                     });
                if (cur_max_iter == cur_res.cend()) {
                    continue;
                }
                if (double cur_score = cur_max_iter->score;
                    max_score < cur_score) {
                    mm_analyzer.sort_result();
                    max_score = cur_score;
                    cur_facility_result = cur_res;
                    cor_suffix_index = i;
                }
            }
        }
        if (cur_facility_result.empty()) {
            continue;
        }

#ifdef LOG_TRACE
        cv::RNG rng(time(0));
        cv::Scalar rand_color(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
        int index = 0;
        for (const MatchRect& mr : cur_facility_result) {
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(mr.rect), rand_color, 2);
            cv::Point text_point(mr.rect.x + 2, mr.rect.y + mr.rect.height / 2);
            cv::putText(m_image_draw, task_name + std::to_string(index++), text_point, 1, 1, cv::Scalar(0, 0, 255), 1);
        }
#endif
        m_result.emplace(key, std::move(cur_facility_result));
    }

    return !m_result.empty();
}