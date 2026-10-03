#pragma once
#include "Vision/VisionHelper.h"

namespace asst
{
class ParadoxDetailAnalyzer : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    std::optional<std::string> analyze() const;
};
}
