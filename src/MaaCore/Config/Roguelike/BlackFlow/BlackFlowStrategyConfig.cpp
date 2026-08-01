#include "BlackFlowStrategyConfig.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace asst
{
namespace
{
using namespace blackflow;

[[noreturn]] void invalid_config(const std::string& message)
{
    throw std::runtime_error("BlackFlow strategy config: " + message);
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

Condition constant_condition(bool value)
{
    Condition condition;
    condition.kind = ConditionKind::Constant;
    condition.constant = value;
    return condition;
}

FactValue parse_fact_value(const json::value& value)
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
        std::vector<std::string> result;
        for (const auto& entry : value.as_array()) {
            if (!entry.is_string()) {
                invalid_config("fact value arrays may contain strings only");
            }
            result.emplace_back(entry.as_string());
        }
        return result;
    }
    invalid_config("fact value must be a boolean, integer, string, or string array");
}

bool fact_value_matches(FactType type, const FactValue& value)
{
    switch (type) {
    case FactType::Boolean:
        return std::holds_alternative<bool>(value);
    case FactType::Integer:
        return std::holds_alternative<std::int64_t>(value);
    case FactType::String:
        return std::holds_alternative<std::string>(value);
    case FactType::StringList:
        return std::holds_alternative<std::vector<std::string>>(value);
    }
    return false;
}

FactType parse_fact_type(const std::string& value)
{
    static const std::unordered_map<std::string, FactType> Values = {
        { "bool", FactType::Boolean },
        { "int", FactType::Integer },
        { "string", FactType::String },
        { "string_list", FactType::StringList },
    };
    const auto found = Values.find(value);
    if (found == Values.end()) {
        invalid_config("unknown fact type: " + value);
    }
    return found->second;
}

FactScope parse_fact_scope(const std::string& value)
{
    static const std::unordered_map<std::string, FactScope> Values = {
        { "run", FactScope::Run },
        { "floor", FactScope::Floor },
        { "page", FactScope::Page },
        { "candidate", FactScope::Candidate },
    };
    const auto found = Values.find(value);
    if (found == Values.end()) {
        invalid_config("unknown fact scope: " + value);
    }
    return found->second;
}

RoutePreference parse_route_preference(const std::string& value)
{
    if (value == "minimize_intermediate_interactions") {
        return RoutePreference::MinimizeIntermediateInteractions;
    }
    invalid_config("unknown route preference: " + value);
}

FactDefinition parse_fact_definition(const json::value& value)
{
    check_keys(value, { "name", "type", "scope", "description" }, { "name", "type", "scope" }, "fact");
    FactDefinition result;
    result.name = value.at("name").as_string();
    result.type = parse_fact_type(value.at("type").as_string());
    result.scope = parse_fact_scope(value.at("scope").as_string());
    result.description = value.get("description", std::string());
    if (result.name.empty()) {
        invalid_config("fact name must not be empty");
    }
    return result;
}

CompareOperator parse_compare_operator(const std::string& value)
{
    static const std::unordered_map<std::string, CompareOperator> Values = {
        { "exists", CompareOperator::Exists },     { "not_exists", CompareOperator::NotExists },
        { "eq", CompareOperator::Equal },          { "ne", CompareOperator::NotEqual },
        { "lt", CompareOperator::Less },           { "le", CompareOperator::LessEqual },
        { "gt", CompareOperator::Greater },        { "ge", CompareOperator::GreaterEqual },
        { "contains", CompareOperator::Contains },
    };
    const auto found = Values.find(value);
    if (found == Values.end()) {
        invalid_config("unknown comparison operator: " + value);
    }
    return found->second;
}

Condition parse_condition(const json::value& value)
{
    if (value.is_boolean()) {
        return constant_condition(value.as_boolean());
    }
    if (!value.is_object()) {
        invalid_config("condition must be a boolean or object");
    }
    const auto& object = value.as_object();
    int forms = 0;
    for (const auto key : { "const", "all", "any", "not", "fact" }) {
        forms += object.contains(key) ? 1 : 0;
    }
    if (forms != 1) {
        invalid_config("condition must contain exactly one condition form");
    }

    if (object.contains("const")) {
        check_keys(value, { "const" }, { "const" }, "constant condition");
        if (!object.at("const").is_boolean()) {
            invalid_config("condition const must be a boolean");
        }
        return constant_condition(object.at("const").as_boolean());
    }
    if (object.contains("all") || object.contains("any")) {
        const bool all = object.contains("all");
        const std::string key = all ? "all" : "any";
        check_keys(value, { key }, { key }, "compound condition");
        if (!object.at(key).is_array()) {
            invalid_config("compound condition children must be an array");
        }
        Condition result;
        result.kind = all ? ConditionKind::All : ConditionKind::Any;
        for (const auto& child : object.at(key).as_array()) {
            result.children.emplace_back(parse_condition(child));
        }
        return result;
    }
    if (object.contains("not")) {
        check_keys(value, { "not" }, { "not" }, "not condition");
        Condition result;
        result.kind = ConditionKind::Not;
        result.children.emplace_back(parse_condition(object.at("not")));
        return result;
    }

    check_keys(value, { "fact", "op", "value" }, { "fact" }, "predicate condition");
    Condition result;
    result.kind = ConditionKind::Predicate;
    result.fact = object.at("fact").as_string();
    result.compare = parse_compare_operator(value.get("op", std::string("exists")));
    const bool needs_value = result.compare != CompareOperator::Exists && result.compare != CompareOperator::NotExists;
    if (needs_value != object.contains("value")) {
        invalid_config("predicate value presence differs from its operator");
    }
    if (needs_value) {
        result.value = parse_fact_value(object.at("value"));
    }
    return result;
}

RuleKind parse_rule_kind(const std::string& value)
{
    static const std::unordered_map<std::string, RuleKind> Values = {
        { "forbid", RuleKind::Forbid },
        { "require", RuleKind::Require },
        { "prefer", RuleKind::Prefer },
        { "tie_break", RuleKind::TieBreak },
    };
    const auto found = Values.find(value);
    if (found == Values.end()) {
        invalid_config("unknown rule kind: " + value);
    }
    return found->second;
}

PolicyTier parse_policy_tier(const std::string& value)
{
    static const std::unordered_map<std::string, PolicyTier> Values = {
        { "legality", PolicyTier::Legality },
        { "safety", PolicyTier::Safety },
        { "strategy_constraint", PolicyTier::StrategyConstraint },
        { "resource_reserve", PolicyTier::ResourceReserve },
        { "preferred_milestone", PolicyTier::PreferredMilestone },
        { "development", PolicyTier::Development },
        { "risk", PolicyTier::Risk },
        { "tie_break", PolicyTier::TieBreak },
    };
    const auto found = Values.find(value);
    if (found == Values.end()) {
        invalid_config("unknown policy tier: " + value);
    }
    return found->second;
}

MilestoneKind parse_milestone_kind(const std::string& value)
{
    static const std::unordered_map<std::string, MilestoneKind> Values = {
        { "preferred", MilestoneKind::Preferred },
        { "opportunistic", MilestoneKind::Opportunistic },
    };
    const auto found = Values.find(value);
    if (found == Values.end()) {
        invalid_config("unknown milestone kind: " + value);
    }
    return found->second;
}

MilestoneCompletion parse_milestone_completion(const std::string& value)
{
    if (value == "visit_count") {
        return MilestoneCompletion::VisitCount;
    }
    if (value == "condition") {
        return MilestoneCompletion::Condition;
    }
    invalid_config("unknown milestone completion mode: " + value);
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
        if (!entry.is_string()) {
            invalid_config(key + " may contain strings only");
        }
        result.emplace_back(entry.as_string());
    }
    return result;
}

Condition optional_condition(const json::value& parent, const std::string& key, bool default_value)
{
    const auto value = parent.find(key);
    return value ? parse_condition(*value) : constant_condition(default_value);
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

std::string optional_page_intent(const json::value& parent)
{
    const std::string result = parent.get("page_intent", std::string());
    if (!result.empty() && !is_valid_page_intent(result)) {
        invalid_config("page_intent must use lower-case dotted semantic identifiers");
    }
    return result;
}

PolicyRule parse_rule(const json::value& value)
{
    check_keys(
        value,
        { "id", "description", "kind", "tier", "rank", "when", "candidate_if", "page_intent" },
        { "id", "kind", "tier", "candidate_if" },
        "rule");
    PolicyRule result;
    result.id = value.at("id").as_string();
    result.description = value.get("description", std::string());
    result.kind = parse_rule_kind(value.at("kind").as_string());
    result.tier = parse_policy_tier(value.at("tier").as_string());
    result.rank = value.get("rank", 0);
    result.when = optional_condition(value, "when", true);
    result.candidate_if = parse_condition(value.at("candidate_if"));
    result.page_intent = optional_page_intent(value);
    if (result.id.empty()) {
        invalid_config("rule id must not be empty");
    }
    return result;
}

ResourceReserve parse_reserve(const json::value& value)
{
    check_keys(
        value,
        { "id", "description", "resource", "minimum", "active_if", "release_if" },
        { "id", "resource", "minimum" },
        "resource reserve");
    ResourceReserve result;
    result.id = value.at("id").as_string();
    result.description = value.get("description", std::string());
    result.resource = value.at("resource").as_string();
    result.minimum = value.at("minimum").as_integer();
    result.active_if = optional_condition(value, "active_if", true);
    result.release_if = optional_condition(value, "release_if", false);
    if (result.id.empty() || result.resource.empty() || result.minimum < 0) {
        invalid_config("resource reserve has an invalid id, resource, or minimum");
    }
    return result;
}

NodeIdentityState parse_identity_state(const std::string& value)
{
    static const std::unordered_map<std::string, NodeIdentityState> Values = {
        { "classified", NodeIdentityState::Classified },
        { "hidden", NodeIdentityState::Hidden },
        { "unclassified", NodeIdentityState::Unclassified },
    };
    const auto found = Values.find(value);
    if (found == Values.end()) {
        invalid_config("unknown node identity state: " + value);
    }
    return found->second;
}

NodeSelector parse_node_selector(const json::value& value)
{
    check_keys(
        value,
        { "node_types", "node_names", "marker_types", "badged", "identity_state", "identity_revealed" },
        {},
        "node selector");
    NodeSelector result;
    for (const std::string& type_name : parse_string_array(value, "node_types")) {
        const auto type = node_type_from_string(type_name);
        if (!type.has_value() || *type == NodeType::Unknown) {
            invalid_config("node selector references an unsupported node type: " + type_name);
        }
        result.node_types.emplace_back(*type);
    }
    std::ranges::sort(result.node_types, {}, [](NodeType type) { return static_cast<int>(type); });
    if (std::ranges::adjacent_find(result.node_types) != result.node_types.end()) {
        invalid_config("node selector node_types contains duplicates");
    }
    result.node_names = parse_string_array(value, "node_names");
    if (std::ranges::any_of(result.node_names, [](const std::string& name) { return name.empty(); })) {
        invalid_config("node selector node_names must not contain empty names");
    }
    std::ranges::sort(result.node_names);
    if (std::ranges::adjacent_find(result.node_names) != result.node_names.end()) {
        invalid_config("node selector node_names contains duplicates");
    }
    result.marker_types = parse_string_array(value, "marker_types");
    if (std::ranges::any_of(result.marker_types, [](const std::string& type) { return type.empty(); })) {
        invalid_config("node selector marker_types must not contain empty types");
    }
    std::ranges::sort(result.marker_types);
    if (std::ranges::adjacent_find(result.marker_types) != result.marker_types.end()) {
        invalid_config("node selector marker_types contains duplicates");
    }
    if (const auto badged = value.find("badged"); badged) {
        if (!badged->is_boolean()) {
            invalid_config("node selector badged must be boolean");
        }
        result.badged = badged->as_boolean();
    }
    if (const auto identity = value.find("identity_state"); identity) {
        if (!identity->is_string()) {
            invalid_config("node selector identity_state must be a string");
        }
        result.identity_state = parse_identity_state(identity->as_string());
    }
    if (const auto revealed = value.find("identity_revealed"); revealed) {
        if (!revealed->is_boolean()) {
            invalid_config("node selector identity_revealed must be boolean");
        }
        result.identity_revealed = revealed->as_boolean();
    }
    if (result.empty()) {
        invalid_config("node selector must contain at least one criterion");
    }
    return result;
}

std::vector<HiddenNodeReveal> parse_hidden_node_reveals(const json::value& value)
{
    if (!value.is_object()) {
        invalid_config("hidden_node_reveals must be an object");
    }
    std::vector<HiddenNodeReveal> result;
    for (const auto& [hidden_type_name, revealed_types] : value.as_object()) {
        const auto hidden_type = node_type_from_string(hidden_type_name);
        if (!hidden_type.has_value() ||
            (*hidden_type != NodeType::HideInvisible && *hidden_type != NodeType::HideBattle)) {
            invalid_config("hidden_node_reveals contains an unsupported hidden node type: " + hidden_type_name);
        }
        if (!revealed_types.is_array()) {
            invalid_config("hidden_node_reveals values must be node type arrays");
        }
        HiddenNodeReveal reveal;
        reveal.hidden_node_type = *hidden_type;
        for (const auto& revealed_type_value : revealed_types.as_array()) {
            if (!revealed_type_value.is_string()) {
                invalid_config("hidden_node_reveals node types must be strings");
            }
            const std::string revealed_type_name = revealed_type_value.as_string();
            const auto revealed_type = node_type_from_string(revealed_type_name);
            if (!revealed_type.has_value() || *revealed_type == NodeType::Unknown ||
                *revealed_type == NodeType::HideInvisible || *revealed_type == NodeType::HideBattle) {
                invalid_config("hidden_node_reveals contains an unsupported revealed node type: " + revealed_type_name);
            }
            reveal.revealed_node_types.emplace_back(*revealed_type);
        }
        std::ranges::sort(reveal.revealed_node_types, {}, [](NodeType type) { return static_cast<int>(type); });
        if (reveal.revealed_node_types.empty() ||
            std::ranges::adjacent_find(reveal.revealed_node_types) != reveal.revealed_node_types.end()) {
            invalid_config("hidden_node_reveals node type arrays must be nonempty and contain no duplicates");
        }
        result.emplace_back(std::move(reveal));
    }
    std::ranges::sort(result, {}, [](const HiddenNodeReveal& reveal) {
        return static_cast<int>(reveal.hidden_node_type);
    });
    if (result.empty() ||
        std::ranges::adjacent_find(result, [](const HiddenNodeReveal& lhs, const HiddenNodeReveal& rhs) {
            return lhs.hidden_node_type == rhs.hidden_node_type;
        }) != result.end()) {
        invalid_config("hidden_node_reveals must be nonempty and contain no duplicate hidden node types");
    }
    return result;
}

Milestone parse_milestone(const json::value& value)
{
    check_keys(
        value,
        { "id",
          "description",
          "kind",
          "completion",
          "floor_window",
          "rank",
          "count",
          "weight",
          "end",
          "minimum_unknown_nodes_revealed",
          "active_if",
          "complete_if",
          "selector",
          "page_intent",
          "reserves",
          "requires" },
        { "id", "floor_window", "selector" },
        "milestone");
    Milestone result;
    result.id = value.at("id").as_string();
    result.description = value.get("description", std::string());
    if (const auto kind = value.find("kind"); kind) {
        result.kind = parse_milestone_kind(kind->as_string());
    }
    result.completion = parse_milestone_completion(value.get("completion", std::string("visit_count")));
    result.rank = value.get("rank", 0);
    result.required_count = value.get("count", 1);
    result.weight = value.get("weight", 1);
    result.end = value.get("end", false);
    result.minimum_unknown_nodes_revealed = value.get("minimum_unknown_nodes_revealed", 0);
    result.active_if = optional_condition(value, "active_if", true);
    result.complete_if = optional_condition(value, "complete_if", false);
    result.selector = parse_node_selector(value.at("selector"));
    result.page_intent = optional_page_intent(value);
    result.reserve_ids = parse_string_array(value, "reserves");
    result.prerequisites = parse_string_array(value, "requires");

    const auto& window = value.at("floor_window");
    if (!window.is_array() || window.as_array().size() != 2 || !window.at(0).is_number() || !window.at(1).is_number()) {
        invalid_config("milestone floor_window must contain two integers");
    }
    result.floor_begin = window.at(0).as_integer();
    result.floor_end = window.at(1).as_integer();
    if ((result.kind == MilestoneKind::None) != result.end) {
        invalid_config("milestone must define exactly one of kind or end");
    }
    if (result.id.empty() || result.floor_begin < 1 || result.floor_end < result.floor_begin ||
        result.required_count < 1 || result.weight < 1 || result.minimum_unknown_nodes_revealed < 0) {
        invalid_config("milestone id, floor window, count, weight, or reveal threshold is invalid");
    }
    if (result.minimum_unknown_nodes_revealed > 0 &&
        std::ranges::find(result.selector.node_types, NodeType::Light) == result.selector.node_types.end()) {
        invalid_config("milestone reveal threshold requires a light node selector");
    }
    if (result.completion == MilestoneCompletion::Condition && !value.find("complete_if")) {
        invalid_config("condition-completed milestone requires complete_if");
    }
    if (result.completion == MilestoneCompletion::VisitCount && value.find("complete_if")) {
        invalid_config("visit-count milestone must not define complete_if");
    }
    return result;
}

GrantedScrap parse_granted_scrap(const json::value& value)
{
    check_keys(value, { "movement", "description", "when" }, { "movement", "when" }, "granted scrap");
    GrantedScrap result;
    result.id = value.at("movement").as_string();
    result.description = value.get("description", std::string());
    const auto& specs = movement_specs();
    const auto spec = std::ranges::find_if(specs, [&](const MovementSpec& candidate) {
        return candidate.kind != MovementKind::Walk && candidate.id == result.id;
    });
    if (spec == specs.end()) {
        invalid_config("granted scrap references unknown processing item: " + result.id);
    }
    if (spec->initial_charges <= 0) {
        invalid_config("granted scrap has no usable charges: " + result.id);
    }
    result.movement = spec->kind;
    result.when = parse_condition(value.at("when"));
    return result;
}

PolicyModule parse_module(const json::value& value)
{
    check_keys(
        value,
        { "id", "description", "route_preferences", "rules", "reserves", "milestones", "granted_scraps" },
        { "id" },
        "module");
    PolicyModule result;
    result.id = value.at("id").as_string();
    result.description = value.get("description", std::string());
    if (result.id.empty()) {
        invalid_config("module id must not be empty");
    }
    for (const std::string& preference : parse_string_array(value, "route_preferences")) {
        const RoutePreference parsed = parse_route_preference(preference);
        if (std::ranges::find(result.route_preferences, parsed) != result.route_preferences.end()) {
            invalid_config("module contains duplicate route preference: " + preference);
        }
        result.route_preferences.emplace_back(parsed);
    }
    if (const auto rules = value.find("rules"); rules) {
        if (!rules->is_array()) {
            invalid_config("module rules must be an array");
        }
        for (const auto& rule : rules->as_array()) {
            result.rules.emplace_back(parse_rule(rule));
        }
    }
    if (const auto reserves = value.find("reserves"); reserves) {
        if (!reserves->is_array()) {
            invalid_config("module reserves must be an array");
        }
        for (const auto& reserve : reserves->as_array()) {
            result.reserves.emplace_back(parse_reserve(reserve));
        }
    }
    if (const auto milestones = value.find("milestones"); milestones) {
        if (!milestones->is_array()) {
            invalid_config("module milestones must be an array");
        }
        for (const auto& milestone : milestones->as_array()) {
            result.milestones.emplace_back(parse_milestone(milestone));
        }
    }
    if (const auto granted = value.find("granted_scraps"); granted) {
        if (!granted->is_array()) {
            invalid_config("module granted_scraps must be an array");
        }
        for (const auto& scrap : granted->as_array()) {
            result.granted_scraps.emplace_back(parse_granted_scrap(scrap));
        }
    }
    return result;
}

StrategyTerminalRule parse_terminal_rule(const json::value& value)
{
    check_keys(
        value,
        { "id", "when", "outcome", "reason", "succeeded", "next_action" },
        { "id", "when", "outcome", "reason", "succeeded" },
        "profile terminal rule");
    StrategyTerminalRule result;
    result.id = value.at("id").as_string();
    result.when = parse_condition(value.at("when"));
    result.outcome = value.at("outcome").as_string();
    result.reason = value.at("reason").as_string();
    result.succeeded = value.at("succeeded").as_boolean();
    result.next_action = value.get("next_action", std::string {});
    if (result.id.empty() || result.outcome.empty() || result.reason.empty()) {
        invalid_config("profile terminal rule id, outcome, and reason must not be empty");
    }
    if (!result.next_action.empty() && result.next_action != "stop_run" &&
        result.next_action != "restart_current_run") {
        invalid_config("profile terminal rule next_action is unsupported: " + result.next_action);
    }
    return result;
}

PolicyProfile parse_profile(const json::value& value)
{
    check_keys(
        value,
        { "id", "description", "modules", "terminal_rules", "failure_action" },
        { "id", "modules" },
        "profile");
    PolicyProfile result;
    result.id = value.at("id").as_string();
    result.description = value.get("description", std::string());
    result.modules = parse_string_array(value, "modules");
    if (const auto rules = value.find("terminal_rules"); rules) {
        if (!rules->is_array()) {
            invalid_config("profile terminal_rules must be an array");
        }
        std::unordered_set<std::string> rule_ids;
        for (const auto& rule : rules->as_array()) {
            StrategyTerminalRule parsed = parse_terminal_rule(rule);
            if (!rule_ids.emplace(parsed.id).second) {
                invalid_config("profile contains duplicate terminal rule: " + parsed.id);
            }
            result.terminal_rules.emplace_back(std::move(parsed));
        }
    }
    result.failure_action = value.get("failure_action", std::string("stop_run"));
    if (result.id.empty() || result.modules.empty() || result.failure_action.empty()) {
        invalid_config("profile id, module list, and failure action must be present");
    }
    if (result.failure_action != "stop_run" && result.failure_action != "restart_current_run") {
        invalid_config("profile failure_action is unsupported: " + result.failure_action);
    }
    std::unordered_set<std::string> unique_modules;
    for (const auto& module : result.modules) {
        if (!unique_modules.emplace(module).second) {
            invalid_config("profile contains duplicate module: " + module);
        }
    }
    return result;
}

void validate_condition(
    const Condition& condition,
    const std::unordered_map<std::string, FactDefinition>& facts,
    bool allow_candidate)
{
    if (condition.kind == ConditionKind::All || condition.kind == ConditionKind::Any ||
        condition.kind == ConditionKind::Not) {
        for (const auto& child : condition.children) {
            validate_condition(child, facts, allow_candidate);
        }
        return;
    }
    if (condition.kind != ConditionKind::Predicate) {
        return;
    }
    const auto definition = facts.find(condition.fact);
    if (definition == facts.end()) {
        invalid_config("condition references undeclared fact: " + condition.fact);
    }
    if (!allow_candidate && definition->second.scope == FactScope::Candidate) {
        invalid_config("non-candidate condition references candidate fact: " + condition.fact);
    }
    if (condition.compare == CompareOperator::Exists || condition.compare == CompareOperator::NotExists) {
        return;
    }
    if (!condition.value.has_value()) {
        invalid_config("condition comparison is missing a value");
    }
    const FactType type = definition->second.type;
    if (condition.compare == CompareOperator::Less || condition.compare == CompareOperator::LessEqual ||
        condition.compare == CompareOperator::Greater || condition.compare == CompareOperator::GreaterEqual) {
        if (type != FactType::Integer || !std::holds_alternative<std::int64_t>(*condition.value)) {
            invalid_config("ordered comparison requires an integer fact and value");
        }
    }
    else if (condition.compare == CompareOperator::Contains) {
        if ((type != FactType::String && type != FactType::StringList) ||
            !std::holds_alternative<std::string>(*condition.value)) {
            invalid_config("contains requires a string or string-list fact and a string value");
        }
    }
    else if (!fact_value_matches(type, *condition.value)) {
        invalid_config("condition value type differs from fact declaration: " + condition.fact);
    }
}

void validate_module(
    const PolicyModule& module,
    const std::unordered_map<std::string, FactDefinition>& facts,
    const std::unordered_map<std::string, ResourceDefinition>& resources)
{
    std::unordered_set<std::string> ids;
    for (const auto& rule : module.rules) {
        if (!ids.emplace("rule:" + rule.id).second) {
            invalid_config("module contains duplicate rule id: " + rule.id);
        }
        validate_condition(rule.when, facts, false);
        validate_condition(rule.candidate_if, facts, true);
    }
    for (const auto& reserve : module.reserves) {
        if (!ids.emplace("reserve:" + reserve.id).second) {
            invalid_config("module contains duplicate reserve id: " + reserve.id);
        }
        if (!resources.contains(reserve.resource)) {
            invalid_config("resource reserve references unknown resource: " + reserve.resource);
        }
        validate_condition(reserve.active_if, facts, false);
        validate_condition(reserve.release_if, facts, true);
    }
    for (const auto& milestone : module.milestones) {
        if (!ids.emplace("milestone:" + milestone.id).second) {
            invalid_config("module contains duplicate milestone id: " + milestone.id);
        }
        validate_condition(milestone.active_if, facts, false);
        validate_condition(milestone.complete_if, facts, false);
    }
    for (const auto& scrap : module.granted_scraps) {
        if (!ids.emplace("granted_scrap:" + scrap.id).second) {
            invalid_config("module contains duplicate granted scrap: " + scrap.id);
        }
        validate_condition(scrap.when, facts, false);
    }
}

template <typename Item>
bool append_unique(
    std::vector<Item>& destination,
    const std::vector<Item>& source,
    std::unordered_set<std::string>& ids,
    std::string* error,
    const std::string& kind)
{
    for (const auto& item : source) {
        if (!ids.emplace(item.id).second) {
            if (error != nullptr) {
                *error = "duplicate " + kind + " id in resolved profile: " + item.id;
            }
            return false;
        }
        destination.emplace_back(item);
    }
    return true;
}

void validate_requirement_cycles(const std::vector<Milestone>& milestones)
{
    std::unordered_map<std::string, const Milestone*> by_id;
    for (const auto& milestone : milestones) {
        by_id.emplace(milestone.id, &milestone);
    }
    std::unordered_map<std::string, int> marks;
    std::function<void(const Milestone&)> visit = [&](const Milestone& milestone) {
        if (marks[milestone.id] == 1) {
            invalid_config("milestone requirement cycle contains: " + milestone.id);
        }
        if (marks[milestone.id] == 2) {
            return;
        }
        marks[milestone.id] = 1;
        for (const auto& required : milestone.prerequisites) {
            visit(*by_id.at(required));
        }
        marks[milestone.id] = 2;
    };
    for (const auto& milestone : milestones) {
        visit(milestone);
    }
}

void validate_profile_definition(
    const PolicyProfile& profile,
    const std::unordered_map<std::string, PolicyModule>& modules,
    const std::unordered_map<std::string, FactDefinition>& facts)
{
    ResolvedPolicy resolved;
    std::unordered_set<std::string> rule_ids;
    std::unordered_set<std::string> reserve_ids;
    std::unordered_set<std::string> milestone_ids;
    std::unordered_set<std::string> granted_scrap_ids;
    std::string error;
    for (const auto& module_id : profile.modules) {
        const auto module = modules.find(module_id);
        if (module == modules.end()) {
            invalid_config("profile references unknown module: " + module_id);
        }
        if (!append_unique(resolved.rules, module->second.rules, rule_ids, &error, "rule") ||
            !append_unique(resolved.reserves, module->second.reserves, reserve_ids, &error, "reserve") ||
            !append_unique(resolved.milestones, module->second.milestones, milestone_ids, &error, "milestone") ||
            !append_unique(
                resolved.granted_scraps,
                module->second.granted_scraps,
                granted_scrap_ids,
                &error,
                "granted scrap")) {
            invalid_config(error);
        }
    }

    std::unordered_map<std::string, const Milestone*> milestones_by_id;
    for (const auto& milestone : resolved.milestones) {
        milestones_by_id.emplace(milestone.id, &milestone);
    }
    for (const auto& milestone : resolved.milestones) {
        for (const auto& reserve : milestone.reserve_ids) {
            if (!reserve_ids.contains(reserve)) {
                invalid_config("milestone references unknown reserve: " + reserve);
            }
        }
        for (const auto& required : milestone.prerequisites) {
            const auto prerequisite = milestones_by_id.find(required);
            if (prerequisite == milestones_by_id.end()) {
                invalid_config("milestone requires unknown milestone: " + required);
            }
            if (prerequisite->second->floor_begin > milestone.floor_end) {
                invalid_config(
                    "milestone prerequisite cannot be completed before its dependent milestone: " + required);
            }
        }
    }
    for (const StrategyTerminalRule& rule : profile.terminal_rules) {
        validate_condition(rule.when, facts, false);
    }
    validate_requirement_cycles(resolved.milestones);
}
} // namespace

const blackflow::FactDefinition* BlackFlowStrategyConfig::get_fact_definition(const std::string& name) const noexcept
{
    const auto found = m_facts.find(name);
    return found == m_facts.end() ? nullptr : &found->second;
}

const blackflow::PolicyModule* BlackFlowStrategyConfig::get_module(const std::string& id) const noexcept
{
    const auto found = m_modules.find(id);
    return found == m_modules.end() ? nullptr : &found->second;
}

const blackflow::PolicyProfile* BlackFlowStrategyConfig::get_profile(const std::string& id) const noexcept
{
    const auto found = m_profiles.find(id);
    return found == m_profiles.end() ? nullptr : &found->second;
}

std::unordered_set<std::string> BlackFlowStrategyConfig::page_intents() const
{
    std::unordered_set<std::string> result;
    for (const auto& [id, module] : m_modules) {
        (void)id;
        for (const auto& rule : module.rules) {
            if (!rule.page_intent.empty()) {
                result.emplace(rule.page_intent);
            }
        }
        for (const auto& milestone : module.milestones) {
            if (!milestone.page_intent.empty()) {
                result.emplace(milestone.page_intent);
            }
        }
    }
    return result;
}

std::optional<blackflow::ResolvedPolicy>
    BlackFlowStrategyConfig::resolve_profile(const std::string& id, std::string* error) const
{
    const blackflow::PolicyProfile* profile = get_profile(id);
    if (profile == nullptr) {
        if (error != nullptr) {
            *error = "unknown BlackFlow strategy profile: " + id;
        }
        return std::nullopt;
    }
    blackflow::ResolvedPolicy result;
    result.profile_id = profile->id;
    result.description = profile->description;
    result.modules = profile->modules;
    result.terminal_rules = profile->terminal_rules;
    result.failure_action = profile->failure_action;
    result.hidden_node_reveals = m_hidden_node_reveals;

    std::unordered_set<std::string> rule_ids;
    std::unordered_set<std::string> reserve_ids;
    std::unordered_set<std::string> milestone_ids;
    std::unordered_set<std::string> granted_scrap_ids;
    for (const auto& module_id : profile->modules) {
        const blackflow::PolicyModule* module = get_module(module_id);
        if (module == nullptr || !append_unique(result.rules, module->rules, rule_ids, error, "rule") ||
            !append_unique(result.reserves, module->reserves, reserve_ids, error, "reserve") ||
            !append_unique(result.milestones, module->milestones, milestone_ids, error, "milestone") ||
            !append_unique(result.granted_scraps, module->granted_scraps, granted_scrap_ids, error, "granted scrap")) {
            if (module == nullptr && error != nullptr) {
                *error = "profile references unknown module: " + module_id;
            }
            return std::nullopt;
        }
        for (const blackflow::RoutePreference preference : module->route_preferences) {
            if (std::ranges::find(result.route_preferences, preference) == result.route_preferences.end()) {
                result.route_preferences.emplace_back(preference);
            }
        }
    }
    return result;
}

bool BlackFlowStrategyConfig::parse_for_test(const json::value& json, std::string* error)
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

bool BlackFlowStrategyConfig::parse(const json::value& json)
{
    check_keys(
        json,
        { "schema_version", "resources", "hidden_node_reveals", "facts", "modules", "profiles" },
        { "schema_version", "resources", "hidden_node_reveals", "facts", "modules", "profiles" },
        "root");
    const int schema_version = json.at("schema_version").as_integer();
    if (schema_version != 7) {
        invalid_config("unsupported schema_version: " + std::to_string(schema_version));
    }
    for (const auto key : { "resources", "facts", "modules", "profiles" }) {
        if (!json.at(key).is_array()) {
            invalid_config(std::string(key) + " must be an array");
        }
    }

    std::unordered_map<std::string, blackflow::ResourceDefinition> resources;
    ResourceRegistry builtins;
    for (const auto& value : json.at("resources").as_array()) {
        check_keys(value, { "id", "description" }, { "id" }, "resource");
        blackflow::ResourceDefinition definition;
        definition.id = value.at("id").as_string();
        definition.description = value.get("description", std::string());
        if (definition.id.empty() || !builtins.contains(definition.id) ||
            !resources.emplace(definition.id, definition).second) {
            invalid_config("resource id is empty, unknown, or duplicated: " + definition.id);
        }
    }

    std::vector<blackflow::HiddenNodeReveal> hidden_node_reveals =
        parse_hidden_node_reveals(json.at("hidden_node_reveals"));

    std::unordered_map<std::string, blackflow::FactDefinition> facts;
    for (const auto& value : json.at("facts").as_array()) {
        auto definition = parse_fact_definition(value);
        if (!facts.emplace(definition.name, definition).second) {
            invalid_config("duplicate fact name: " + definition.name);
        }
    }

    std::unordered_map<std::string, blackflow::PolicyModule> modules;
    for (const auto& value : json.at("modules").as_array()) {
        auto module = parse_module(value);
        validate_module(module, facts, resources);
        if (!modules.emplace(module.id, module).second) {
            invalid_config("duplicate module id: " + module.id);
        }
    }

    std::unordered_map<std::string, blackflow::PolicyProfile> profiles;
    for (const auto& value : json.at("profiles").as_array()) {
        auto profile = parse_profile(value);
        const std::string id = profile.id;
        if (!profiles.emplace(id, profile).second) {
            invalid_config("duplicate profile id: " + id);
        }
    }
    if (resources.empty() || facts.empty() || modules.empty() || profiles.empty()) {
        invalid_config("resources, facts, modules, and profiles must not be empty");
    }
    for (const auto& [id, profile] : profiles) {
        (void)id;
        validate_profile_definition(profile, modules, facts);
    }

    m_schema_version = schema_version;
    m_resources = std::move(resources);
    m_hidden_node_reveals = std::move(hidden_node_reveals);
    m_facts = std::move(facts);
    m_modules = std::move(modules);
    m_profiles = std::move(profiles);
    return true;
}
} // namespace asst
