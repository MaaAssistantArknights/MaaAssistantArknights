#pragma once
#include "MultiMatchImageAnalyzer.h"
#include "OcrWithPreprocessImageAnalyzer.h"

namespace asst
{
    class OcrWithFlagTemplImageAnalyzer : public OcrWithPreprocessImageAnalyzer
    {
    public:
        OcrWithFlagTemplImageAnalyzer() = default;
        OcrWithFlagTemplImageAnalyzer(const cv::Mat& image);
        virtual ~OcrWithFlagTemplImageAnalyzer() override = default;

        virtual void set_image(const cv::Mat& image) override;
        virtual void set_roi(const Rect& roi) noexcept override;

        virtual bool analyze() override;

        virtual const std::vector<TextRect>& get_result() const noexcept override;
        virtual std::vector<TextRect>& get_result() noexcept override;

        void set_task_info(const std::string& templ_task_name, const std::string& ocr_task_name);
        void set_flag_rect_move(Rect flag_rect_move);

    protected:
        using OcrWithPreprocessImageAnalyzer::set_task_info;

        MultiMatchImageAnalyzer m_multi_match_image_analyzer;
        Rect m_flag_rect_move;
        std::vector<TextRect> m_all_result;
    };
}
