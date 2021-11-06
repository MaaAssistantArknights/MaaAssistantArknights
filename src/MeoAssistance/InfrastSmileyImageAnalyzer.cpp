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

#include "InfrastSmileyImageAnalyzer.h"

#include "AsstUtils.hpp"
#include "MultiMatchImageAnalyzer.h"
#include "Resource.h"

bool asst::InfrastSmileyImageAnalyzer::analyze() {
    const static std::unordered_map<InfrastSmileyType, std::string> smiley_map = {
        { InfrastSmileyType::Rest, "InfrastSmileyOnRest" },
        { InfrastSmileyType::Work, "InfrastSmileyOnWork" },
        { InfrastSmileyType::Distract, "InfrastSmileyOnDistract" }
    };

    m_result.clear();

    MultiMatchImageAnalyzer mm_analyzer(m_image);

    decltype(m_result) temp_result;
    for (const auto& [type, task_name] : smiley_map) {
        const auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(resource.task().task_ptr(task_name));
        mm_analyzer.set_task_info(*task_ptr);
        mm_analyzer.set_roi(m_roi);
        if (!mm_analyzer.analyze()) {
            continue;
        }
        auto& res = mm_analyzer.get_result();
        for (const MatchRect& mr : res) {
            temp_result.emplace_back(InfrastSmileyInfo{ type, mr.rect });
#ifdef LOG_TRACE
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(mr.rect), cv::Scalar(0, 0, 255), 2);
#endif
        }
    }
    m_result = std::move(temp_result);
    return true;
}