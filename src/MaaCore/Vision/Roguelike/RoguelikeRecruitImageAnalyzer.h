#pragma once
#include "Vision/VisionHelper.h"

#include "Common/AsstBattleDef.h"

namespace asst
{
class RoguelikeRecruitImageAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~RoguelikeRecruitImageAnalyzer() noexcept override = default;

    bool analyze();

    const auto& get_result() const noexcept { return m_result; }

private:
    int match_elite(const Rect& raw_roi);
    static int match_level(const cv::Mat& image, const Rect& raw_roi);

    std::vector<battle::roguelike::Recruitment> m_result;
};
}
