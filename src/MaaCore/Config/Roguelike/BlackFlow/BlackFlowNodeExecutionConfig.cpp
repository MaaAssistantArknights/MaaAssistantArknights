#include "BlackFlowNodeExecutionConfig.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <tuple>
#include <unordered_set>
#include <utility>

#include "Config/Roguelike/BlackFlow/BlackFlowStrategyConfig.h"
#include "Config/TaskData.h"

namespace asst
{
namespace
{
using namespace blackflow;

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

std::vector<std::string> parse_string_array(const json::value& parent, const std::string& key)
{
    std::vector<std::string> result;
    const auto value = parent.find(key);
    if (!value) {
        return result;
    }
    if (!value->is_array()) {
        invalid_config(key + " must be an array");
    }
    for (const auto& entry : value->as_array()) {
        if (!entry.is_string() || entry.as_string().empty()) {
            invalid_config(key + " may contain non-empty strings only");
        }
        result.emplace_back(entry.as_string());
    }
    std::ranges::sort(result);
    if (std::ranges::adjacent_find(result) != result.end()) {
        invalid_config(key + " contains duplicate values");
    }
    return result;
}

std::pair<int, int> parse_floor_window(const json::value& value)
{
    const auto window = value.find("floor_window");
    if (!window) {
        return { 1, std::numeric_limits<int>::max() };
    }
    if (!window->is_array() || window->as_array().size() != 2 || !window->at(0).is_number() ||
        !window->at(1).is_number()) {
        invalid_config("floor_window must contain two integers");
    }
    const int floor_begin = window->at(0).as_integer();
    const int floor_end = window->at(1).as_integer();
    if (floor_begin < 1 || floor_end < floor_begin) {
        invalid_config("floor_window must be a positive, ascending range");
    }
    return { floor_begin, floor_end };
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

std::vector<NodeType> parse_node_types(const json::value& value)
{
    std::vector<NodeType> result;
    for (const std::string& name : parse_string_array(value, "node_types")) {
        const auto type = node_type_from_string(name);
        if (!type.has_value()) {
            invalid_config("route references an unsupported node type: " + name);
        }
        result.emplace_back(*type);
    }
    std::ranges::sort(result, {}, [](NodeType type) { return static_cast<int>(type); });
    return result;
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

NodeExecutionRoute parse_route(const json::value& value)
{
    check_keys(
        value,
        { "id",
          "page_intent",
          "floor_window",
          "node_types",
          "event_names",
          "rank",
          "alias",
          "task",
          "completion_task" },
        { "id", "page_intent", "alias", "task", "completion_task" },
        "route");

    NodeExecutionRoute result;
    result.id = value.at("id").as_string();
    result.page_intent = value.at("page_intent").as_string();
    std::tie(result.floor_begin, result.floor_end) = parse_floor_window(value);
    result.node_types = parse_node_types(value);
    result.event_names = parse_string_array(value, "event_names");
    result.rank = value.get("rank", 0);
    result.alias = value.at("alias").as_string();
    result.task = value.at("task").as_string();
    result.completion_task = value.at("completion_task").as_string();

    if (result.id.empty() || !is_valid_page_intent(result.page_intent) || result.rank < 0) {
        invalid_config(
            "route id must be present, page_intent must be lower-case dotted text, and rank must be non-negative");
    }
    if (result.alias != "BlackFlow@Roguelike@NodeDispatchAction") {
        invalid_config("route alias must use BlackFlow@Roguelike@NodeDispatchAction");
    }
    validate_task_alias(result.alias, "route alias");
    validate_task_alias(result.task, "route task");
    validate_task_alias(result.completion_task, "route completion_task");
    return result;
}

NodeSignalValue parse_signal_value(const json::value& value)
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

NodeSignalKind parse_signal_kind(const std::string& value)
{
    if (value == "set") {
        return NodeSignalKind::Set;
    }
    if (value == "add") {
        return NodeSignalKind::Add;
    }
    if (value == "capture_integer") {
        return NodeSignalKind::CaptureInteger;
    }
    invalid_config("unknown signal kind: " + value);
}

NodeStrategySignal parse_signal(const json::value& value)
{
    check_keys(
        value,
        { "kind", "fact", "value", "source", "minimum", "maximum" },
        { "kind", "fact" },
        "strategy signal");

    NodeStrategySignal result;
    result.kind = parse_signal_kind(value.at("kind").as_string());
    result.fact = value.at("fact").as_string();
    if (result.fact.empty()) {
        invalid_config("strategy signal fact must not be empty");
    }

    const bool has_value = value.as_object().contains("value");
    const bool has_source = value.as_object().contains("source");
    if (result.kind == NodeSignalKind::Set) {
        if (!has_value || has_source || value.find("minimum") || value.find("maximum")) {
            invalid_config("set signal requires value and does not accept source or integer bounds");
        }
        result.value = parse_signal_value(value.at("value"));
    }
    else if (result.kind == NodeSignalKind::Add) {
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

    const FactDefinition* definition = BlackFlowStrategy.get_fact_definition(result.fact);
    if (definition == nullptr) {
        invalid_config("strategy signal references an undeclared fact: " + result.fact);
    }
    if (definition->scope == FactScope::Candidate) {
        invalid_config("strategy signal cannot write a candidate-scoped fact: " + result.fact);
    }
    if ((result.kind == NodeSignalKind::Add || result.kind == NodeSignalKind::CaptureInteger) &&
        definition->type != FactType::Integer) {
        invalid_config("integer strategy signal requires an integer fact: " + result.fact);
    }
    if (result.kind == NodeSignalKind::Set && result.value.has_value()) {
        const bool type_matches =
            (definition->type == FactType::Boolean && std::holds_alternative<bool>(*result.value)) ||
            (definition->type == FactType::Integer && std::holds_alternative<std::int64_t>(*result.value)) ||
            (definition->type == FactType::String && std::holds_alternative<std::string>(*result.value)) ||
            (definition->type == FactType::StringList &&
             std::holds_alternative<std::vector<std::string>>(*result.value));
        if (!type_matches) {
            invalid_config("set strategy signal value type differs from fact: " + result.fact);
        }
    }
    return result;
}

NodeProgress parse_node_progress(const std::string& value)
{
    if (value == "active") {
        return NodeProgress::Active;
    }
    if (value == "completed") {
        return NodeProgress::Completed;
    }
    if (value == "removed") {
        return NodeProgress::Removed;
    }
    invalid_config("unknown node progress: " + value);
}

NodeStateUpdate parse_node_update(const json::value& value)
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

    NodeStateUpdate result;
    if (const auto progress = value.find("progress"); progress) {
        result.progress = parse_node_progress(progress->as_string());
    }
    if (const auto type = value.find("actual_type"); type) {
        const auto parsed = node_type_from_string(type->as_string());
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

NodeTaskResultKind parse_task_result_kind(const std::string& value)
{
    if (value == "intermediate") {
        return NodeTaskResultKind::Intermediate;
    }
    if (value == "page_completed") {
        return NodeTaskResultKind::PageCompleted;
    }
    invalid_config("unknown task result kind: " + value);
}

NodeTaskResult parse_task_result(const json::value& value)
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

    NodeTaskResult result;
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
    if (result.terminate && result.kind != NodeTaskResultKind::PageCompleted) {
        invalid_config("terminating task result must complete its page");
    }
    if (result.kind == NodeTaskResultKind::PageCompleted && result.redispatch) {
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
    check_keys(
        json,
        { "schema_version", "routes", "task_results" },
        { "schema_version", "routes", "task_results" },
        "root");
    const int schema_version = json.at("schema_version").as_integer();
    if (schema_version != 3) {
        invalid_config("unsupported schema_version: " + std::to_string(schema_version));
    }
    if (!json.at("routes").is_array() || !json.at("task_results").is_array()) {
        invalid_config("routes and task_results must be arrays");
    }

    std::vector<NodeExecutionRoute> routes;
    std::unordered_set<std::string> route_ids;
    for (const auto& value : json.at("routes").as_array()) {
        auto route = parse_route(value);
        if (!route_ids.emplace(route.id).second) {
            invalid_config("duplicate route id: " + route.id);
        }
        routes.emplace_back(std::move(route));
    }
    if (routes.empty()) {
        invalid_config("routes must not be empty");
    }
    for (const std::string& intent : BlackFlowStrategy.page_intents()) {
        if (std::ranges::none_of(routes, [&](const auto& route) { return route.page_intent == intent; })) {
            invalid_config("strategy page_intent has no execution route: " + intent);
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
                invalid_config("same-rank routes overlap: " + routes[left].id + " and " + routes[right].id);
            }
        }
    }

    std::unordered_map<std::string, NodeTaskResult> task_results;
    for (const auto& value : json.at("task_results").as_array()) {
        auto result = parse_task_result(value);
        const std::string task = result.task;
        if (!task_results.emplace(task, std::move(result)).second) {
            invalid_config("duplicate task result: " + task);
        }
    }

    m_schema_version = schema_version;
    m_routes = std::move(routes);
    m_task_results = std::move(task_results);
    return true;
}
} // namespace asst
