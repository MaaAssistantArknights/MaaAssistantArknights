#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst
{
    class InfrastClueVacancyImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~InfrastClueVacancyImageAnalyzer() = default;
        InfrastClueVacancyImageAnalyzer(const cv::Mat image, const Rect& roi) = delete;

        virtual bool analyze() override;
        constexpr static int MaxNumOfClue = 7;

        void set_to_be_analyzed(std::vector<std::string> to_be_analyzed) noexcept
        {
            m_to_be_analyzed = std::move(to_be_analyzed);
        }

        const std::unordered_map<std::string, Rect>& get_vacancy() const noexcept
        {
            return m_clue_vacancy;
        }

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

        std::vector<std::string> m_to_be_analyzed;
        std::unordered_map<std::string, Rect> m_clue_vacancy;
    };
}
