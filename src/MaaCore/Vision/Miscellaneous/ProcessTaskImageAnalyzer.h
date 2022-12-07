#pragma once
#include "Vision/AbstractImageAnalyzer.h"

#include <memory>
#include <string>
#include <vector>

#include "Common/AsstTypes.h"

namespace asst
{
    class OcrImageAnalyzer;
    class OcrWithPreprocessImageAnalyzer;
    class MatchImageAnalyzer;
    class Assistant;

    class ProcessTaskImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        ProcessTaskImageAnalyzer(const cv::Mat& image, std::vector<std::string> tasks_name, Assistant* inst);
        virtual ~ProcessTaskImageAnalyzer() override;

        virtual bool analyze() override;
        virtual void set_image(const cv::Mat& image) override;

        void set_tasks(std::vector<std::string> tasks_name);

        std::shared_ptr<TaskInfo> get_result() const noexcept { return m_result; }
        const Rect& get_rect() const noexcept { return m_result_rect; }

        ProcessTaskImageAnalyzer& operator=(const ProcessTaskImageAnalyzer&) = delete;
        ProcessTaskImageAnalyzer& operator=(ProcessTaskImageAnalyzer&&) = delete;

    private:
        // 该分析器不支持外部设置ROI
        using AbstractImageAnalyzer::set_roi;
        bool match_analyze(const std::shared_ptr<TaskInfo>& task_ptr);
        bool ocr_analyze(const std::shared_ptr<TaskInfo>& task_ptr);
        void reset() noexcept;

        std::unique_ptr<OcrImageAnalyzer> m_ocr_analyzer;
        std::unique_ptr<OcrWithPreprocessImageAnalyzer> m_ocr_with_preprocess_analyzer;
        std::unique_ptr<MatchImageAnalyzer> m_match_analyzer;
        std::vector<std::string> m_tasks_name;
        std::shared_ptr<TaskInfo> m_result = nullptr;
        Rect m_result_rect;
        // std::vector<TextRect> m_ocr_cache;
    };
}
