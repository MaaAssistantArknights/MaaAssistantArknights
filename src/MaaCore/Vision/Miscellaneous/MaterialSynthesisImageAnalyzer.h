#pragma once

#include "Vision/VisionHelper.h"

namespace asst
{
class MaterialSynthesisImageAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~MaterialSynthesisImageAnalyzer() override = default;

    bool analyze();

    const MatchRect& get_result() const noexcept { return m_result; }

private:
    MatchRect m_result;
};
}
