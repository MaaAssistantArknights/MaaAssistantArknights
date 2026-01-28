#pragma once
#include "Common/AsstInfrastDef.h"
#include "Vision/VisionHelper.h"

#include <vector>

namespace asst
{
class InfrastIntelligentDormitoryAnalyzer : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~InfrastIntelligentDormitoryAnalyzer() override = default;

    bool analyze();

    auto get_result() const noexcept -> const std::vector<infrast::InfrastDormInfo>& { return m_result; }

protected:
    std::vector<infrast::InfrastDormInfo> m_result;

private:
    void analyze_room(const Rect& anchor_rect);
    void analyze_slot(int slot_idx, const Rect& anchor_rect, infrast::InfrastDormInfo& room_info);
    double identify_smiley_and_mood(const Rect& slot_roi);
    double calculate_mood_ratio(const Rect& smiley_rect);
};
}
