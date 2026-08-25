#pragma once

#include <string>
#include <vector>

namespace asst
{
enum class OperatorDevelopmentAction
{
    Elite,
    Skills,
    Mastery,
};

struct OperatorDevelopmentTarget
{
    std::string name;
    OperatorDevelopmentAction action = OperatorDevelopmentAction::Elite;
    int target = 0;
    int skill = 0;
};

using OperatorDevelopmentPlan = std::vector<OperatorDevelopmentTarget>;
} // namespace asst
