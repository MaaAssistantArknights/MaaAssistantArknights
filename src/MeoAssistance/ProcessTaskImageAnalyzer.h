#pragma once
#include "AbstractImageAnalyzer.h"

#include <vector>
#include <string>
#include <memory>

#include "AsstDef.h"

namespace asst {
    class OcrImageAnalyzer;
    class MatchImageAnalyzer;

    class ProcessTaskImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        ProcessTaskImageAnalyzer(const cv::Mat& image, std::vector<std::string> tasks_name);
        virtual ~ProcessTaskImageAnalyzer();

        virtual bool analyze() override;
        virtual void set_image(const cv::Mat& image, const Rect& roi = Rect()) override;
        virtual void set_roi(const Rect& roi) noexcept override;

        void set_tasks(std::vector<std::string> tasks_name) {
            m_tasks_name = std::move(tasks_name);
        }
        std::shared_ptr<TaskInfo> get_result() const noexcept {
            return m_result;
        }
        const Rect& get_rect() const noexcept {
            return m_result_rect;
        }
    protected:
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
