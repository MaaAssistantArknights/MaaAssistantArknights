#pragma once
#include "Vision/VisionHelper.h"

#include "Common/AsstInfrastDef.h"

MAA_VISION_NS_BEGIN

class InfrastSmileyAnalyzer : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~InfrastSmileyAnalyzer() override = default;

    bool analyze();

    auto get_result() const noexcept -> const std::vector<infrast::Smiley>& { return m_result; }

protected:
    std::vector<infrast::Smiley> m_result;
};

MAA_VISION_NS_END
