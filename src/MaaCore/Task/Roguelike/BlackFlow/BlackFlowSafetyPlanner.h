#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace asst::blackflow
{
using SafetyStateId = std::uint32_t;

inline constexpr int UnreachableActionPointRequirement = std::numeric_limits<int>::max() / 4;

struct SafetyState
{
    SafetyStateId id = 0;
    bool safe_exit = false;
    std::string debug_name;
};

struct SafetyOutcome
{
    SafetyStateId successor = 0;
    int action_point_gain = 0;
};

struct SafetyAction
{
    std::string id;
    SafetyStateId source = 0;
    int action_point_cost = 0;
    int minimum_action_points_to_start = 1;
    std::vector<SafetyOutcome> outcomes;
};

struct SafetyProblem
{
    std::vector<SafetyState> states;
    std::vector<SafetyAction> actions;
};

struct SafetySolution
{
    std::unordered_map<SafetyStateId, int> required_action_points;
    std::unordered_map<SafetyStateId, std::string> selected_actions;
    std::unordered_map<SafetyStateId, std::size_t> proof_depth;

    [[nodiscard]] int requirement(SafetyStateId state) const noexcept;
    [[nodiscard]] const std::string* action(SafetyStateId state) const noexcept;
    [[nodiscard]] std::optional<std::size_t> depth(SafetyStateId state) const noexcept;
    [[nodiscard]] bool certifies(SafetyStateId state, int current_action_points) const noexcept;
};

struct SafetySolveResult
{
    std::optional<SafetySolution> solution;
    std::string error;

    [[nodiscard]] explicit operator bool() const noexcept { return solution.has_value(); }
};

struct SafetyAssessment
{
    int required_action_points = UnreachableActionPointRequirement;
    std::optional<std::string> first_action;
    std::optional<std::size_t> proof_depth;
    SafetySolution solution;
};

class SafetyPlanner
{
public:
    [[nodiscard]] SafetySolveResult solve(const SafetyProblem& problem) const;

    [[nodiscard]] std::optional<SafetyAssessment>
        assess(const SafetyProblem& problem, SafetyStateId initial, std::string* error = nullptr) const;

private:
    [[nodiscard]] bool validate(const SafetyProblem& problem, std::string* error) const;
};
} // namespace asst::blackflow

