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

#include "InfrastClueVacancyImageAnalyzer.h"

#include "AsstUtils.hpp"
#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "Resource.h"

bool asst::InfrastClueVacancyImageAnalyzer::analyze() {
    const static std::string clue_vacancy = "InfrastClueVacancy";

    MatchImageAnalyzer analyzer(m_image);
    for (const std::string& suffix : m_to_be_analyzed) {
        const auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
            resource.task().task_ptr(clue_vacancy + suffix));
        analyzer.set_task_info(*task_ptr);
        if (!analyzer.analyze()) {
            log.trace("no", clue_vacancy, suffix);
            continue;
        }
        Rect rect = analyzer.get_result().rect;
        log.trace("has", clue_vacancy, suffix);
#ifdef LOG_TRACE
        cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(rect), cv::Scalar(0, 0, 255), 2);
        cv::putText(m_image_draw, suffix, cv::Point(rect.x, rect.y + 1), 0, 1, cv::Scalar(0, 0, 255), 2);
#endif
        m_clue_vacancy.emplace(suffix, rect);
    }

    return !m_clue_vacancy.empty();
}