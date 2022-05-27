#pragma once
#include "MultiMatchImageAnalyzer.h"
#include "OcrWithPreprocessImageAnalyzer.h"

namespace asst
{
    class OcrWithFlagTemplImageAnalyzer : public OcrWithPreprocessImageAnalyzer
    {
    public:
        OcrWithFlagTemplImageAnalyzer() = default;
        OcrWithFlagTemplImageAnalyzer(const cv::Mat image);
        OcrWithFlagTemplImageAnalyzer(const cv::Mat image, const Rect& roi);
        virtual ~OcrWithFlagTemplImageAnalyzer() = default;

        virtual void set_image(const cv::Mat image);
        virtual void set_image(const cv::Mat image, const Rect& roi);
        virtual void set_roi(const Rect& roi) noexcept;

        virtual bool analyze() override;

        virtual const std::vector<TextRect>& get_result() const noexcept override;
        virtual std::vector<TextRect>& get_result() noexcept override;

        void set_task_info(std::string templ_task_name, std::string ocr_task_name);
        void set_flag_rect_move(Rect flag_rect_move);

    protected:

        MultiMatchImageAnalyzer m_multi_match_image_analyzer;
        Rect m_flag_rect_move;
        std::vector<TextRect> m_all_result;
    };
}
