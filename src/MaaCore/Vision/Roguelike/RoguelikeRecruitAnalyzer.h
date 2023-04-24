#pragma once
#include "Vision/VisionHelper.h"

#include "Common/AsstBattleDef.h"

MAA_VISION_NS_BEGIN

class RoguelikeRecruitAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~RoguelikeRecruitAnalyzer() noexcept override = default;

    bool analyze() override;

    const auto& get_result() const noexcept { return m_result; }

private:
    int match_elite(const Rect& raw_roi);
    static int match_level(const cv::Mat& image, const Rect& raw_roi);

    std::vector<battle::roguelike::Recruitment> m_result;
};

MAA_VISION_NS_END
