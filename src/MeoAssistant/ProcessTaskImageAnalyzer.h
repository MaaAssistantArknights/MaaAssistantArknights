#pragma once
#include "AbstractImageAnalyzer.h"

#include <memory>
#include <string>
#include <vector>

#include "AsstTypes.h"

namespace asst
{
    class OcrImageAnalyzer;
    class MatchImageAnalyzer;
    class RuntimeStatus;

    class ProcessTaskImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        ProcessTaskImageAnalyzer(const cv::Mat image, const Rect& roi) = delete;
        ProcessTaskImageAnalyzer(const cv::Mat& image, std::vector<std::string> tasks_name);
        virtual ~ProcessTaskImageAnalyzer() override;

        virtual bool analyze() override;
        void set_image(const cv::Mat image);

        void set_tasks(std::vector<std::string> tasks_name);
        void set_status(std::shared_ptr<RuntimeStatus> status) noexcept;

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
        bool match_analyze(const std::shared_ptr<TaskInfo>& task_ptr);
        bool ocr_analyze(const std::shared_ptr<TaskInfo>& task_ptr);
        void reset() noexcept;

        std::unique_ptr<OcrImageAnalyzer> m_ocr_analyzer;
        std::unique_ptr<MatchImageAnalyzer> m_match_analyzer;
        std::vector<std::string> m_tasks_name;
        std::shared_ptr<TaskInfo> m_result = nullptr;
        std::shared_ptr<RuntimeStatus> m_status = nullptr;
        Rect m_result_rect;
        //std::vector<TextRect> m_ocr_cache;
    };
}
