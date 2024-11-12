#pragma once
#include "Vision/VisionHelper.h"

namespace asst
{
class InfrastClueVacancyImageAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~InfrastClueVacancyImageAnalyzer() override = default;
    InfrastClueVacancyImageAnalyzer(const cv::Mat& image, const Rect& roi) = delete;

    bool analyze();
    static constexpr int MaxNumOfClue = 7;

    void set_to_be_analyzed(std::vector<std::string> to_be_analyzed) noexcept
    {
        m_to_be_analyzed = std::move(to_be_analyzed);
    }

    const std::unordered_map<std::string, Rect>& get_vacancy() const noexcept { return m_clue_vacancy; }

private:
    // 该分析器不支持外部设置ROI
    using VisionHelper::set_roi;

    std::vector<std::string> m_to_be_analyzed;
    std::unordered_map<std::string, Rect> m_clue_vacancy;
};
}
