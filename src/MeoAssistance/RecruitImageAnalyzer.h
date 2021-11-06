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

#include "AsstDef.h"

namespace asst {
    class RecruitImageAnalyzer final : public AbstractImageAnalyzer {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        RecruitImageAnalyzer(const cv::Mat& image, const Rect& roi) = delete;
        virtual ~RecruitImageAnalyzer() = default;

        virtual bool analyze() override;

        const std::vector<TextRect>& get_tags_result() const noexcept {
            return m_tags_result;
        }
        const std::vector<Rect>& get_set_time_rect() const noexcept {
            return m_set_time_rect;
        }

    private:
        // 该分析器不支持外部设置ROI
        virtual void set_roi(const Rect& roi) noexcept override {
            AbstractImageAnalyzer::set_roi(roi);
        }
        virtual void set_image(const cv::Mat& image, const Rect& roi) {
            AbstractImageAnalyzer::set_image(image, roi);
        }
        bool tags_analyze();
        bool time_analyze();

        std::vector<TextRect> m_tags_result;
        std::vector<Rect> m_set_time_rect;
    };
}
