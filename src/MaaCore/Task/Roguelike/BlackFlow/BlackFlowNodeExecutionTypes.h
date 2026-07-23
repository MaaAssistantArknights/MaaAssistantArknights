#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "BlackFlowModel.h"

namespace asst::blackflow
{
struct NodeExecutionContext
{
    int floor = 0;
    NodeType node_type = NodeType::Unknown;
    std::string event_name;
    std::string page_intent = "default";
};

struct NodeExecutionRoute
{
    std::string id;
    std::string page_intent;
    int floor_begin = 1;
    int floor_end = std::numeric_limits<int>::max();
    std::vector<NodeType> node_types;
    std::vector<std::string> event_names;
    int rank = 0;
    std::string alias;
    std::string task;

    [[nodiscard]] bool matches(const NodeExecutionContext& context) const noexcept
    {
        if (context.floor < floor_begin || context.floor > floor_end || context.page_intent != page_intent) {
            return false;
        }
        if (!node_types.empty() && std::ranges::find(node_types, context.node_type) == node_types.end()) {
            return false;
        }
        return event_names.empty() || std::ranges::binary_search(event_names, context.event_name);
    }
};

enum class NodeSignalKind
{
    Set,
    Add,
    CaptureInteger,
};

using NodeSignalValue = std::variant<bool, std::int64_t, std::string, std::vector<std::string>>;

struct NodeStrategySignal
{
    NodeSignalKind kind = NodeSignalKind::Set;
    std::string fact;
    std::optional<NodeSignalValue> value;
    std::string source;
    int minimum = std::numeric_limits<int>::min();
    int maximum = std::numeric_limits<int>::max();
};

struct NodeStateUpdate
{
    std::optional<NodeProgress> progress;
    std::optional<NodeType> actual_type;
    std::optional<std::string> actual_name;
    std::string actual_name_source;
    std::optional<bool> identity_revealed;
    std::optional<bool> repeatable;
    std::optional<bool> becomes_empty;
};

enum class NodeTaskResultKind
{
    Intermediate,
    PageCompleted,
};

struct NodeTaskResult
{
    std::string task;
    NodeTaskResultKind kind = NodeTaskResultKind::Intermediate;
    NodeStateUpdate node;
    std::vector<NodeStrategySignal> signals;
    std::string outcome_code;
    std::string termination_reason;
    bool terminate = false;
    bool succeeded = false;
    bool redispatch = false;
};
} // namespace asst::blackflow
