#pragma once
#include "AbstractImageAnalyzer.h"

#include <memory>
#include <string>
#include <vector>

#include "AsstDef.h"

namespace asst
{
    class OcrImageAnalyzer;
    class MatchImageAnalyzer;

    class ProcessTaskImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        ProcessTaskImageAnalyzer(const cv::Mat image, const Rect& roi) = delete;
        ProcessTaskImageAnalyzer(const cv::Mat image, std::vector<std::string> tasks_name);
        virtual ~ProcessTaskImageAnalyzer();

        virtual bool analyze() override;
        virtual void set_image(const cv::Mat image) override;

        void set_tasks(std::vector<std::string> tasks_name)
        {
            m_tasks_name = std::move(tasks_name);
        }
        std::shared_ptr<TaskInfo> get_result() const noexcept
        {
            return m_result;
        }
        const Rect& get_rect() const noexcept
        {
            return m_result_rect;
        }

        ProcessTaskImageAnalyzer& operator=(const ProcessTaskImageAnalyzer&) = delete;
        ProcessTaskImageAnalyzer& operator=(ProcessTaskImageAnalyzer&&) = delete;

    private:
        // 该分析器不支持外部设置ROI
        virtual void set_roi(const Rect& roi) noexcept override
        {
            AbstractImageAnalyzer::set_roi(roi);
        }
        virtual void set_image(const cv::Mat image, const Rect& roi)
        {
            AbstractImageAnalyzer::set_image(image, roi);
        }
        bool match_analyze(std::shared_ptr<TaskInfo> task_ptr);
        bool ocr_analyze(std::shared_ptr<TaskInfo> task_ptr);
        void reset() noexcept;

        std::unique_ptr<OcrImageAnalyzer> m_ocr_analyzer;
        std::unique_ptr<MatchImageAnalyzer> m_match_analyzer;
        std::vector<std::string> m_tasks_name;
        std::shared_ptr<TaskInfo> m_result;
        Rect m_result_rect;
        //std::vector<TextRect> m_ocr_cache;
    };
}
