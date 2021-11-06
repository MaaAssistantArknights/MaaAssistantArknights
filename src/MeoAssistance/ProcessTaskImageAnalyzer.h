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

#include <memory>
#include <string>
#include <vector>

#include "AsstDef.h"

namespace asst {
    class OcrImageAnalyzer;
    class MatchImageAnalyzer;

    class ProcessTaskImageAnalyzer final : public AbstractImageAnalyzer {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        ProcessTaskImageAnalyzer(const cv::Mat& image, const Rect& roi) = delete;
        ProcessTaskImageAnalyzer(const cv::Mat& image, std::vector<std::string> tasks_name);
        virtual ~ProcessTaskImageAnalyzer();

        virtual bool analyze() override;
        virtual void set_image(const cv::Mat& image) override;

        void set_tasks(std::vector<std::string> tasks_name) {
            m_tasks_name = std::move(tasks_name);
        }
        std::shared_ptr<TaskInfo> get_result() const noexcept {
            return m_result;
        }
        const Rect& get_rect() const noexcept {
            return m_result_rect;
        }

    private:
        // 该分析器不支持外部设置ROI
        virtual void set_roi(const Rect& roi) noexcept override {
            AbstractImageAnalyzer::set_roi(roi);
        }
        virtual void set_image(const cv::Mat& image, const Rect& roi) {
            AbstractImageAnalyzer::set_image(image, roi);
        }
        bool match_analyze(std::shared_ptr<TaskInfo> task_ptr);
        bool ocr_analyze(std::shared_ptr<TaskInfo> task_ptr);
        void reset() noexcept;

        std::unique_ptr<OcrImageAnalyzer> m_ocr_analyzer = nullptr;
        std::unique_ptr<MatchImageAnalyzer> m_match_analyzer = nullptr;
        std::vector<std::string> m_tasks_name;
        std::shared_ptr<TaskInfo> m_result;
        Rect m_result_rect;
        std::vector<TextRect> m_ocr_cache;
    };
}
