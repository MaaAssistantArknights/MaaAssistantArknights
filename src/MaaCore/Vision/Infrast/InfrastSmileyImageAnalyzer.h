#pragma once
#include "Vision/VisionHelper.h"

#include "Common/AsstInfrastDef.h"

namespace asst
{
class InfrastSmileyImageAnalyzer : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~InfrastSmileyImageAnalyzer() override = default;

    bool analyze();

    auto get_result() const noexcept -> const std::vector<infrast::Smiley>& { return m_result; }

protected:
    std::vector<infrast::Smiley> m_result;
};
}
