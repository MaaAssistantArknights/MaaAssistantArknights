#pragma once
#include "Vision/VisionHelper.h"
#include "Common/AsstInfrastDef.h"

#include <vector>

namespace asst
{
class InfrastIntelligentAnalyzer : public VisionHelper
{
public:

    using VisionHelper::VisionHelper;
    virtual ~InfrastIntelligentAnalyzer() override = default;

    bool analyze();

    auto get_result() const noexcept -> const std::vector<infrast::InfrastRoomInfo>& { return m_result; }

protected:
    // 存储分析结果
    std::vector<infrast::InfrastRoomInfo> m_result;
};
}