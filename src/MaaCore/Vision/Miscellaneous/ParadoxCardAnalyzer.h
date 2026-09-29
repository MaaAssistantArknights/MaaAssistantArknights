#pragma once
#include "Vision/VisionHelper.h"

namespace asst
{
struct ParadoxCard
{
    Rect rect;
    bool completed = false;
    bool unlocked = false;
};

class ParadoxCardAnalyzer : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    std::optional<std::vector<ParadoxCard>> analyze() const;
};
}
