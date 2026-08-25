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
namespace
{

[[noreturn]] void invalid_config(const std::string& message)
{
    throw std::runtime_error("BlackFlow node execution config: " + message);
}

void check_keys(
    const json::value& value,
    std::initializer_list<std::string_view> allowed,
    std::initializer_list<std::string_view> required,
    std::string_view context)
{
    if (!value.is_object()) {
        invalid_config(std::string(context) + " must be an object");
    }
    const std::unordered_set<std::string_view> allowed_set(allowed);
    for (const auto& [key, ignored] : value.as_object()) {
        (void)ignored;
        if (!allowed_set.contains(key)) {
            invalid_config(std::string(context) + " contains unknown field: " + key);
        }
    }
    for (const std::string_view key : required) {
        if (!value.as_object().contains(std::string(key))) {
            invalid_config(std::string(context) + " is missing field: " + std::string(key));
        }
    }
}

bool is_valid_page_intent(std::string_view value)
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

void validate_task_alias(const std::string& task, std::string_view field)
{
    if (task.empty()) {
        invalid_config(std::string(field) + " must not be empty");
    }
    if (Task.get(task) == nullptr) {
        invalid_config(std::string(field) + " references an unknown ProcessTask alias: " + task);
    }
}

blackflow::NodeSignalValue parse_signal_value(const json::value& value)
{
    if (value.is_boolean()) {
        return value.as_boolean();
    }
    if (value.is_number()) {
        return static_cast<std::int64_t>(value.as_integer());
    }
    if (value.is_string()) {
        return value.as_string();
    }
    if (value.is_array()) {
        std::vector<std::string> strings;
        for (const auto& entry : value.as_array()) {
            if (!entry.is_string()) {
                invalid_config("signal value arrays may contain strings only");
            }
            strings.emplace_back(entry.as_string());
        }
        return strings;
    }
    invalid_config("signal value must be a boolean, integer, string, or string array");
}

blackflow::NodeSignalKind parse_signal_kind(const std::string& value)
{
    if (value == "set") {
        return blackflow::NodeSignalKind::Set;
    }
    if (value == "add") {
        return blackflow::NodeSignalKind::Add;
    }
    if (value == "capture_integer") {
        return blackflow::NodeSignalKind::CaptureInteger;
    }
    invalid_config("unknown signal kind: " + value);
}

blackflow::NodeStrategySignal parse_signal(const json::value& value)
{
    check_keys(
        value,
        { "kind", "fact", "value", "source", "minimum", "maximum" },
        { "kind", "fact" },
        "strategy signal");

    blackflow::NodeStrategySignal result;
    result.kind = parse_signal_kind(value.at("kind").as_string());
    result.fact = value.at("fact").as_string();
    if (result.fact.empty()) {
        invalid_config("strategy signal fact must not be empty");
    }

    const bool has_value = value.as_object().contains("value");
    const bool has_source = value.as_object().contains("source");
    if (result.kind == blackflow::NodeSignalKind::Set) {
        if (!has_value || has_source || value.find("minimum") || value.find("maximum")) {
            invalid_config("set signal requires value and does not accept source or integer bounds");
        }
        result.value = parse_signal_value(value.at("value"));
    }
    else if (result.kind == blackflow::NodeSignalKind::Add) {
        if (!has_value || has_source || value.find("minimum") || value.find("maximum")) {
            invalid_config("add signal requires an integer value and does not accept source or integer bounds");
        }
        result.value = parse_signal_value(value.at("value"));
        if (!std::holds_alternative<std::int64_t>(*result.value)) {
            invalid_config("add signal value must be an integer");
        }
    }
    else {
        if (has_value || !has_source) {
            invalid_config("capture_integer signal requires source and does not accept value");
        }
        result.source = value.at("source").as_string();
        if (result.source != "details.result.text") {
            invalid_config("capture_integer signal source must be details.result.text");
        }
        result.minimum = value.get("minimum", std::numeric_limits<int>::min());
        result.maximum = value.get("maximum", std::numeric_limits<int>::max());
        if (result.minimum > result.maximum) {
            invalid_config("capture_integer signal minimum exceeds maximum");
        }
    }

    const blackflow::FactDefinition* definition = BlackFlowStrategy.get_fact_definition(result.fact);
    if (definition == nullptr) {
        invalid_config("strategy signal references an undeclared fact: " + result.fact);
    }
    if (definition->scope == blackflow::FactScope::Candidate) {
        invalid_config("strategy signal cannot write a candidate-scoped fact: " + result.fact);
    }
    if ((result.kind == blackflow::NodeSignalKind::Add || result.kind == blackflow::NodeSignalKind::CaptureInteger) &&
        definition->type != blackflow::FactType::Integer) {
        invalid_config("integer strategy signal requires an integer fact: " + result.fact);
    }
    if (result.kind == blackflow::NodeSignalKind::Set && result.value.has_value()) {
        const bool type_matches =
            (definition->type == blackflow::FactType::Boolean && std::holds_alternative<bool>(*result.value)) ||
            (definition->type == blackflow::FactType::Integer && std::holds_alternative<std::int64_t>(*result.value)) ||
            (definition->type == blackflow::FactType::String && std::holds_alternative<std::string>(*result.value)) ||
            (definition->type == blackflow::FactType::StringList &&
             std::holds_alternative<std::vector<std::string>>(*result.value));
        if (!type_matches) {
            invalid_config("set strategy signal value type differs from fact: " + result.fact);
        }
    }
    return result;
}

blackflow::NodeProgress parse_node_progress(const std::string& value)
{
    if (value == "active") {
        return blackflow::NodeProgress::Active;
    }
    if (value == "completed") {
        return blackflow::NodeProgress::Completed;
    }
    if (value == "removed") {
        return blackflow::NodeProgress::Removed;
    }
    invalid_config("unknown node progress: " + value);
}

blackflow::NodeStateUpdate parse_node_update(const json::value& value)
{
    check_keys(
        value,
        { "progress",
          "actual_type",
          "actual_name",
          "actual_name_source",
          "identity_revealed",
          "repeatable",
          "becomes_empty" },
        {},
        "node update");

    blackflow::NodeStateUpdate result;
    if (const auto progress = value.find("progress"); progress) {
        result.progress = parse_node_progress(progress->as_string());
    }
    if (const auto type = value.find("actual_type"); type) {
        const auto parsed = blackflow::node_type_from_string(type->as_string());
        if (!parsed.has_value()) {
            invalid_config("node update contains an unsupported actual_type: " + type->as_string());
        }
        result.actual_type = *parsed;
    }
    const auto actual_name = value.find("actual_name");
    const auto actual_name_source = value.find("actual_name_source");
    if (actual_name && actual_name_source) {
        invalid_config("node update cannot define actual_name and actual_name_source together");
    }
    if (actual_name) {
        result.actual_name = actual_name->as_string();
        if (result.actual_name->empty()) {
            invalid_config("node update actual_name must not be empty");
        }
    }
    if (actual_name_source) {
        result.actual_name_source = actual_name_source->as_string();
        if (result.actual_name_source != "details.result.text") {
            invalid_config("node update actual_name_source must be details.result.text");
        }
    }
    if (const auto revealed = value.find("identity_revealed"); revealed) {
        result.identity_revealed = revealed->as_boolean();
    }
    if (const auto repeatable = value.find("repeatable"); repeatable) {
        result.repeatable = repeatable->as_boolean();
    }
    if (const auto empty = value.find("becomes_empty"); empty) {
        result.becomes_empty = empty->as_boolean();
    }
    return result;
}

blackflow::NodeTaskResultKind parse_task_result_kind(const std::string& value)
{
    if (value == "intermediate") {
        return blackflow::NodeTaskResultKind::Intermediate;
    }
    if (value == "page_completed") {
        return blackflow::NodeTaskResultKind::PageCompleted;
    }
    invalid_config("unknown task result kind: " + value);
}

blackflow::NodeTaskResult parse_task_result(const json::value& value)
{
    check_keys(
        value,
        { "task",
          "result_kind",
          "node",
          "signals",
          "outcome",
          "terminate",
          "succeeded",
          "redispatch",
          "termination_reason" },
        { "task", "result_kind" },
        "task result");

    blackflow::NodeTaskResult result;
    result.task = value.at("task").as_string();
    result.kind = parse_task_result_kind(value.at("result_kind").as_string());
    validate_task_alias(result.task, "task result task");
    if (const auto node = value.find("node"); node) {
        result.node = parse_node_update(*node);
    }
    if (const auto signals = value.find("signals"); signals) {
        if (!signals->is_array()) {
            invalid_config("task result signals must be an array");
        }
        for (const auto& signal : signals->as_array()) {
            result.signals.emplace_back(parse_signal(signal));
        }
    }
    result.outcome_code = value.get("outcome", std::string());
    result.terminate = value.get("terminate", false);
    result.succeeded = value.get("succeeded", false);
    result.redispatch = value.get("redispatch", false);
    result.termination_reason = value.get("termination_reason", std::string());
    if (result.terminate && result.kind != blackflow::NodeTaskResultKind::PageCompleted) {
        invalid_config("terminating task result must complete its page");
    }
    if (result.kind == blackflow::NodeTaskResultKind::PageCompleted && result.redispatch) {
        invalid_config("page-completed task result cannot request redispatch");
    }
    if (result.terminate && !value.find("succeeded")) {
        invalid_config("terminating task result requires succeeded");
    }
    if (!result.terminate && value.find("succeeded")) {
        invalid_config("non-terminating task result must not define succeeded");
    }
    if (result.terminate && (result.outcome_code.empty() || result.termination_reason.empty())) {
        invalid_config("terminating task result requires outcome and termination_reason");
    }
    if (!result.terminate && !result.outcome_code.empty()) {
        invalid_config("non-terminating task result must not define outcome");
    }
    if (!result.terminate && !result.termination_reason.empty()) {
        invalid_config("non-terminating task result must not have termination_reason");
    }
    return result;
}
} // namespace

const blackflow::NodeExecutionRoute*
    BlackFlowNodeExecutionConfig::resolve_route(const blackflow::NodeExecutionContext& context) const noexcept
{
    const auto found = std::ranges::find_if(m_routes, [&](const auto& route) { return route.matches(context); });
    return found == m_routes.end() ? nullptr : &*found;
}

const blackflow::NodeTaskResult* BlackFlowNodeExecutionConfig::get_task_result(const std::string& task) const noexcept
{
    const auto found = m_task_results.find(task);
    return found == m_task_results.end() ? nullptr : &found->second;
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
    check_keys(
        json,
        { "schema_version", "routes", "task_results", "preview_names" },
        { "schema_version", "routes", "task_results", "preview_names" },
        "root");
    const int schema_version = json.at("schema_version").as_integer();
    if (schema_version != 3) {
        LogError << __FUNCTION__ << "unsupported schema_version:" << schema_version;
        return false;
    }
    if (!json.at("task_results").is_array()) {
        LogError << __FUNCTION__ << "task_results must be arrays";
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
    for (const auto& value : json.at("task_results").as_array()) {
        auto result = parse_task_result(value);
        const std::string task = result.task;
        if (!task_results.emplace(task, std::move(result)).second) {
            LogError << __FUNCTION__ << "duplicate task result:" << task;
            ret = false;
        }
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

    m_schema_version = schema_version;
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
        if (route.id.empty() || !is_valid_page_intent(route.page_intent) || route.rank < 0) {
            LogError << __FUNCTION__
                     << "route id must be present, page_intent must be lower-case dotted text, and rank must be "
                        "non-negative";
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
