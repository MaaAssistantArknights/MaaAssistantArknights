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
    class InfrastClueVacancyImageAnalyzer final : public AbstractImageAnalyzer {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~InfrastClueVacancyImageAnalyzer() = default;
        InfrastClueVacancyImageAnalyzer(const cv::Mat& image, const Rect& roi) = delete;

        virtual bool analyze() override;
        constexpr static int MaxNumOfClue = 7;

        void set_to_be_analyzed(std::vector<std::string> to_be_analyzed) noexcept {
            m_to_be_analyzed = std::move(to_be_analyzed);
        }

        const std::unordered_map<std::string, Rect>& get_vacancy() const noexcept {
            return m_clue_vacancy;
        }

    private:
        // 该分析器不支持外部设置ROI
        virtual void set_roi(const Rect& roi) noexcept override {
            AbstractImageAnalyzer::set_roi(roi);
        }
        virtual void set_image(const cv::Mat& image, const Rect& roi) {
            AbstractImageAnalyzer::set_image(image, roi);
        }

        std::vector<std::string> m_to_be_analyzed;
        std::unordered_map<std::string, Rect> m_clue_vacancy;
    };
}
