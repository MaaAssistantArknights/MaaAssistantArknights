#pragma once

#include <string>
#include <vector>

namespace asst
{
enum class AutoRaiseAction
{
    Elite,
    Skills,
    Mastery,
};

struct AutoRaiseTarget
{
    std::string name;
    AutoRaiseAction action = AutoRaiseAction::Elite;
    int target = 0;
    int skill = 0;
};

using AutoRaisePlan = std::vector<AutoRaiseTarget>;
} // namespace asst
