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

#include "InfrastClueImageAnalyzer.h"

#include "MultiMatchImageAnalyzer.h"
#include "Resource.h"

bool asst::InfrastClueImageAnalyzer::analyze() {
    clue_detect();
    if (m_need_detailed) {
        clue_analyze();
    }

    return !m_result.empty();
}

bool asst::InfrastClueImageAnalyzer::clue_detect() {
    MultiMatchImageAnalyzer clue_analyzer(m_image);
    const auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("InfrastClue"));
    clue_analyzer.set_task_info(*task_ptr);
    clue_analyzer.set_roi(m_roi); // 改任务以外部设置的roi为准
    if (!clue_analyzer.analyze()) {
        return false;
    }
    clue_analyzer.sort_result();
    for (const auto& res : clue_analyzer.get_result()) {
        m_result.emplace_back(res.rect, std::string());
    }

    return !m_result.empty();
}

bool asst::InfrastClueImageAnalyzer::clue_analyze() {
    // todo
    return false;
}