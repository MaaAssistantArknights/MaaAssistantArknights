#include "BlackFlowNodeExecutionConfig.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <tuple>
#include <unordered_set>
#include <utility>

#include "Config/Roguelike/BlackFlow/BlackFlowStrategyConfig.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

namespace asst
{
const std::optional<std::reference_wrapper<const blackflow::NodeExecutionRoute>>
    BlackFlowNodeExecutionConfig::resolve_route(const blackflow::NodeExecutionContext& context) const noexcept
{
    const auto found = std::ranges::find_if(m_routes, [&](const auto& route) { return route.matches(context); });
    if (found == m_routes.end()) {
        return std::nullopt;
    }
    return std::cref(*found);
}

const std::optional<std::reference_wrapper<const blackflow::NodeTaskResult>>
    BlackFlowNodeExecutionConfig::get_task_result(const std::string& task) const noexcept
{
    const auto found = m_task_results.find(task);
    if (found == m_task_results.end()) {
        return std::nullopt;
    }
    return std::cref(found->second);
}

std::optional<blackflow::NodeType>
    BlackFlowNodeExecutionConfig::preview_node_type(const std::string& text) const noexcept
{
    const auto found = m_preview_name_types.find(text);
    return found == m_preview_name_types.end() ? std::nullopt : std::optional<blackflow::NodeType>(found->second);
}

bool BlackFlowNodeExecutionConfig::parse_for_test(const json::value& json, std::string* error)
{
    try {
        return parse(json);
    }
    catch (const std::exception& exception) {
        if (error != nullptr) {
            *error = exception.what();
        }
        return false;
    }
}

bool BlackFlowNodeExecutionConfig::parse(const json::value& json)
{
    LogTraceFunction;
    bool ret = true;
    const auto schema_version = json.find<int>("schema_version");
    if (!schema_version || *schema_version != 3) {
        LogError << __FUNCTION__ << "unsupported schema_version";
        return false;
    }
    std::vector<blackflow::NodeExecutionRoute> routes;
    std::unordered_set<std::string> route_ids;
    if (!parse_route(json, routes, route_ids)) {
        ret = false;
    }
    if (routes.empty()) {
        LogError << __FUNCTION__ << "routes must not be empty";
        ret = false;
    }
    // 里程碑的层段与路由的层段是两份配置，只能在这里对齐。里程碑在某一层活跃却没有覆盖该层的
    // 路由时，走到节点就无从分派，本局会以节点分派失败结束，因此逐层校验而不只看意图是否存在。
    for (const auto& window : BlackFlowStrategy.page_intent_windows()) {
        for (int floor = window.floor_begin; floor <= window.floor_end; ++floor) {
            const bool covered = std::ranges::any_of(routes, [&](const auto& route) {
                return route.page_intent == window.intent && floor >= route.floor_begin && floor <= route.floor_end;
            });
            if (!covered) {
                LogError << __FUNCTION__ << "strategy page_intent has no execution route on floor" << floor << ":"
                         << window.intent;
                ret = false;
            }
        }
    }
    std::ranges::stable_sort(routes, [](const auto& left, const auto& right) {
        if (left.rank != right.rank) {
            return left.rank < right.rank;
        }
        return left.id < right.id;
    });
    const auto overlaps = [](const auto& left, const auto& right) {
        if (left.rank != right.rank || left.page_intent != right.page_intent || left.floor_end < right.floor_begin ||
            right.floor_end < left.floor_begin) {
            return false;
        }
        const auto list_overlaps = [](const auto& lhs, const auto& rhs) {
            if (lhs.empty() || rhs.empty()) {
                return true;
            }
            return std::ranges::any_of(lhs, [&](const auto& value) { return std::ranges::binary_search(rhs, value); });
        };
        return list_overlaps(left.node_types, right.node_types) && list_overlaps(left.event_names, right.event_names);
    };
    for (std::size_t left = 0; left < routes.size(); ++left) {
        for (std::size_t right = left + 1; right < routes.size(); ++right) {
            if (overlaps(routes[left], routes[right])) {
                LogError << __FUNCTION__ << "same-rank routes overlap:" << routes[left].id << " and "
                         << routes[right].id;
                ret = false;
            }
        }
    }

    std::unordered_map<std::string, blackflow::NodeTaskResult> task_results;
    if (!parse_task_results(json, task_results)) {
        ret = false;
    }

    std::vector<std::string> preview_names;
    std::unordered_map<std::string, blackflow::NodeType> preview_name_types;
    if (!parse_preview_name(json, preview_names, preview_name_types)) {
        ret = false;
    }
    if (preview_names.empty()) {
        LogError << __FUNCTION__ << "preview_names must not be empty";
        ret = false;
    }
    std::ranges::sort(preview_names);

    m_schema_version = *schema_version;
    m_routes = std::move(routes);
    m_task_results = std::move(task_results);
    m_preview_names = std::move(preview_names);
    m_preview_name_types = std::move(preview_name_types);
    return ret;
}

bool BlackFlowNodeExecutionConfig::parse_route(
    const json::value& json,
    std::vector<blackflow::NodeExecutionRoute>& routes,
    std::unordered_set<std::string>& route_ids) const
{
    const auto& route_opt = json.find<std::vector<NodeExecutionRouteDto>>("routes");
    if (!route_opt) {
        LogError << __FUNCTION__ << "missing routes, or format is error";
        return false;
    }
    bool ret = true;
    for (auto& route : *route_opt) {
        if (!verify_page_intent(route.page_intent) || route.rank < 0) {
            LogError << __FUNCTION__ << "page_intent must be lower-case dotted text, and rank must be non-negative";
            ret = false;
            continue;
        }
        if (!route_ids.emplace(route.id).second) {
            LogError << __FUNCTION__ << "duplicate route id:" << route.id;
            ret = false;
            continue;
        }

        blackflow::NodeExecutionRoute node {
            .id = route.id,
            .page_intent = route.page_intent,
            .floor_begin = route.floor_window[0],
            .floor_end = route.floor_window[1],
            .event_names = route.event_names,
            .rank = route.rank,
            .alias = route.alias,
            .task = route.task,
            .completion_task = route.completion_task,
        };
        if (!verify_non_empty(node.event_names, "event_names") ||
            !sort_and_check_unique(node.event_names, "event_names")) {
            ret = false;
        }
        if (!parse_node_types(route.node_types, node.node_types)) {
            ret = false;
        }
        if (node.alias != "BlackFlow@Roguelike@NodeDispatchAction") {
            LogError << __FUNCTION__ << "route alias must use BlackFlow@Roguelike@NodeDispatchAction";
            ret = false;
        }
        if (!verify_task(node.alias, "route alias") || !verify_task(node.task, "route task") ||
            !verify_task(node.completion_task, "route completion_task")) {
            ret = false;
        }
        routes.emplace_back(std::move(node));
    }
    return ret;
}

bool BlackFlowNodeExecutionConfig::parse_task_results(
    const json::value& json,
    std::unordered_map<std::string, blackflow::NodeTaskResult>& task_results) const
{
    const auto& opt = json.find<std::vector<NodeTaskResultDto>>("task_results");
    if (!opt) {
        LogError << __FUNCTION__ << "missing task_results, or format is error";
        return false;
    }

    bool ret = true;
    for (auto& task : *opt) {
        blackflow::NodeStateUpdate node {
            .progress = task.node.progress,
            .actual_type = task.node.actual_type,
            .actual_name = task.node.actual_name,
            .actual_name_source = task.node.actual_name_source,
            .identity_revealed = task.node.identity_revealed,
            .repeatable = task.node.repeatable,
            .becomes_empty = task.node.becomes_empty,
        };

        std::vector<blackflow::NodeStrategySignal> signals;
        for (auto& signal : task.signals) {
            signals.emplace_back(
                blackflow::NodeStrategySignal {
                    .kind = signal.kind,
                    .fact = signal.fact,
                    .value = signal.value,
                    .source = signal.source,
                    .minimum = signal.minimum,
                    .maximum = signal.maximum,
                });
        }

        blackflow::NodeTaskResult result {
            .task = task.task,
            .kind = task.kind,
            .node = node,
            .signals = signals,
            .outcome_code = task.outcome_code,
            .termination_reason = task.termination_reason,
            .terminate = task.terminate,
            .succeeded = task.succeeded,
            .redispatch = task.redispatch,
        };
        if (!verify_task(result.task, "task result task")) {
            ret = false;
            continue;
        }
        if (!task_results.emplace(task.task, std::move(result)).second) {
            LogError << __FUNCTION__ << "duplicate task result:" << task.task;
            ret = false;
        }
    }
    return ret;
}

bool BlackFlowNodeExecutionConfig::parse_preview_name(
    const json::value& json,
    std::vector<std::string>& preview_names,
    std::unordered_map<std::string, blackflow::NodeType>& preview_name_types) const
{
    auto preview_names_opt = json.find<std::vector<NodePreviewName>>("preview_names");
    if (!preview_names_opt) {
        LogError << __FUNCTION__ << "missing preview_names, or format is error";
        return false;
    }
    bool ret = true;
    for (auto& value : *preview_names_opt) {
        if (value.name.empty()) {
            LogError << __FUNCTION__ << "preview name text must not be empty";
            ret = false;
            continue;
        }
        const auto& type = blackflow::node_type_from_string(value.node_type);
        if (!type) {
            LogError << __FUNCTION__ << "preview name references an unsupported node type:" << value.node_type;
            ret = false;
            continue;
        }
        else if (!preview_name_types.emplace(value.name, *type).second) {
            LogError << __FUNCTION__ << "duplicate preview name:" << value.name;
            ret = false;
            continue;
        }
        preview_names.emplace_back(std::move(value.name));
    }
    return ret;
}

bool BlackFlowNodeExecutionConfig::verify_page_intent(std::string_view value) const
{
    if (value.empty()) {
        return false;
    }
    bool segment_start = true;
    for (const char character : value) {
        if (character == '.') {
            if (segment_start) {
                return false;
            }
            segment_start = true;
            continue;
        }
        const bool lower = character >= 'a' && character <= 'z';
        const bool digit = character >= '0' && character <= '9';
        if ((!lower && !digit && character != '_') || (segment_start && !lower)) {
            return false;
        }
        segment_start = false;
    }
    return !segment_start;
}

bool BlackFlowNodeExecutionConfig::parse_node_types(
    const std::vector<std::string>& value,
    std::vector<blackflow::NodeType>& out) const
{
    if (!verify_non_empty(value, "node_types")) {
        return false;
    }
    for (const std::string& name : value) {
        const auto type = blackflow::node_type_from_string(name);
        if (!type.has_value()) {
            LogError << __FUNCTION__ << "route references an unsupported node type:" << name;
            return false;
        }
        out.emplace_back(*type);
    }
    std::ranges::sort(out, {}, [](blackflow::NodeType type) { return static_cast<int>(type); });
    if (std::ranges::adjacent_find(out) != out.end()) {
        LogError << __FUNCTION__ << "node_types contains duplicate values";
        return false;
    }
    return true;
}

bool BlackFlowNodeExecutionConfig::verify_non_empty(const std::vector<std::string>& value, const std::string& key) const
{
    if (std::ranges::any_of(value, [](const std::string& str) { return str.empty(); })) {
        LogError << __FUNCTION__ << key << "may contain non-empty strings only";
        return false;
    }
    return true;
}

bool BlackFlowNodeExecutionConfig::sort_and_check_unique(std::vector<std::string>& value, const std::string& key) const
{
    std::ranges::sort(value);
    if (std::ranges::adjacent_find(value) != value.end()) {
        LogError << __FUNCTION__ << key << "contains duplicate values";
        return false;
    }
    return true;
}

bool BlackFlowNodeExecutionConfig::verify_task(const std::string& task, const std::string& field) const
{
    if (task.empty()) {
        LogError << __FUNCTION__ << field << "must not be empty";
        return false;
    }
    if (Task.get(task) == nullptr) {
        LogError << __FUNCTION__ << field << "references an unknown ProcessTask alias:" << task;
        return false;
    }
    return true;
}
} // namespace asst
