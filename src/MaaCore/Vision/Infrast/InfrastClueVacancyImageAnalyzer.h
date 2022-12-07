#pragma once
#include "Vision/AbstractImageAnalyzer.h"

namespace asst
{
    class InfrastClueVacancyImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~InfrastClueVacancyImageAnalyzer() override = default;
        InfrastClueVacancyImageAnalyzer(const cv::Mat& image, const Rect& roi) = delete;

        virtual bool analyze() override;
        static constexpr int MaxNumOfClue = 7;

        void set_to_be_analyzed(std::vector<std::string> to_be_analyzed) noexcept
        {
            m_to_be_analyzed = std::move(to_be_analyzed);
        }

        const std::unordered_map<std::string, Rect>& get_vacancy() const noexcept { return m_clue_vacancy; }

    private:
        // 该分析器不支持外部设置ROI
        using AbstractImageAnalyzer::set_roi;

        std::vector<std::string> m_to_be_analyzed;
        std::unordered_map<std::string, Rect> m_clue_vacancy;
    };
}
