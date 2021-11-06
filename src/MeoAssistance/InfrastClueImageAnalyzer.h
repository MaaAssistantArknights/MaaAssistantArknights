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
    class InfrastClueImageAnalyzer : public AbstractImageAnalyzer {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~InfrastClueImageAnalyzer() = default;

        virtual bool analyze() override;

        const std::vector<std::pair<Rect, std::string>>& get_result() const noexcept {
            return m_result;
        }

    protected:
        bool clue_detect();
        bool clue_analyze();

        bool m_need_detailed = false; // 是否需要详细分析（线索号）；false时只检测，不识别
        std::vector<std::pair<Rect, std::string>> m_result;
    };
}
