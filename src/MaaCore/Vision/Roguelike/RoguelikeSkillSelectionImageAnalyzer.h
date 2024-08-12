#pragma once
#include "Vision/VisionHelper.h"

namespace asst
{
class RoguelikeSkillSelectionImageAnalyzer final : public VisionHelper
{
public:
    static constexpr size_t MaxNumOfSkills = 3;

public:
    using VisionHelper::VisionHelper;
    virtual ~RoguelikeSkillSelectionImageAnalyzer() override = default;

    bool analyze();

    const auto& get_result() const noexcept { return m_result; }

    bool get_team_full() const noexcept { return m_team_full; }

private:
    // 该分析器不支持外部设置ROI
    using VisionHelper::set_roi;

    std::string name_analyze(const Rect& roi);
    std::vector<Rect> skill_analyze(const Rect& roi);
    void team_full_analyze();

    std::unordered_map<std::string, std::vector<Rect>> m_result; // 干员名 : 技能的位置（多个技能）
    bool m_team_full = false;
};
}
