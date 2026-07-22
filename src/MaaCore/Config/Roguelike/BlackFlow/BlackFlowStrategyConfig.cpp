#include "BlackFlowStrategyConfig.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include "Config/TaskData.h"

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
        { "mandatory_milestone", PolicyTier::MandatoryMilestone },
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
        { "mandatory", MilestoneKind::Mandatory },
        { "preferred", MilestoneKind::Preferred },
        { "opportunistic", MilestoneKind::Opportunistic },
    };
    const auto found = Values.find(value);
    if (found == Values.end()) {
        invalid_config("unknown milestone kind: " + value);
    }
    return found->second;
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

PolicyRule parse_rule(const json::value& value)
{
    check_keys(
        value,
        { "id", "description", "kind", "tier", "rank", "when", "target" },
        { "id", "kind", "tier", "target" },
        "rule");
    PolicyRule result;
    result.id = value.at("id").as_string();
    result.description = value.get("description", std::string());
    result.kind = parse_rule_kind(value.at("kind").as_string());
    result.tier = parse_policy_tier(value.at("tier").as_string());
    result.rank = value.get("rank", 0);
    result.when = optional_condition(value, "when", true);
    result.target = parse_condition(value.at("target"));
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

Milestone parse_milestone(const json::value& value)
{
    check_keys(
        value,
        { "id",
          "description",
          "kind",
          "floor_window",
          "rank",
          "active_if",
          "complete_if",
          "target",
          "reserves",
          "requires",
          "successors" },
        { "id", "kind", "target" },
        "milestone");
    Milestone result;
    result.id = value.at("id").as_string();
    result.description = value.get("description", std::string());
    result.kind = parse_milestone_kind(value.at("kind").as_string());
    result.rank = value.get("rank", 0);
    result.active_if = optional_condition(value, "active_if", true);
    result.complete_if = optional_condition(value, "complete_if", false);
    result.target = parse_condition(value.at("target"));
    result.reserve_ids = parse_string_array(value, "reserves");
    result.prerequisites = parse_string_array(value, "requires");
    result.successors = parse_string_array(value, "successors");
    if (const auto window = value.find("floor_window"); window) {
        if (!window->is_array() || window->as_array().size() != 2) {
            invalid_config("milestone floor_window must contain two integers");
        }
        result.floor_begin = window->at(0).as_integer();
        result.floor_end = window->at(1).as_integer();
    }
    if (result.id.empty() || result.floor_begin < 1 || result.floor_end < result.floor_begin) {
        invalid_config("milestone id or floor window is invalid");
    }
    return result;
}

PolicyModule parse_module(const json::value& value)
{
    check_keys(value, { "id", "description", "rules", "reserves", "milestones" }, { "id" }, "module");
    PolicyModule result;
    result.id = value.at("id").as_string();
    result.description = value.get("description", std::string());
    if (result.id.empty()) {
        invalid_config("module id must not be empty");
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
    return result;
}

PageRoute parse_page_route(const json::value& value)
{
    check_keys(value, { "id", "alias", "task", "rank", "when" }, { "id", "alias", "task" }, "page route");
    PageRoute result;
    result.id = value.at("id").as_string();
    result.alias = value.at("alias").as_string();
    result.task = value.at("task").as_string();
    result.rank = value.get("rank", 0);
    result.when = optional_condition(value, "when", true);
    if (result.id.empty() || result.alias.empty() || result.task.empty()) {
        invalid_config("page route id, alias, and task must not be empty");
    }
    return result;
}

PolicyProfile parse_profile(const json::value& value, std::vector<PageRoute>* page_routes)
{
    check_keys(
        value,
        { "id", "description", "modules", "failure_action", "page_routes" },
        { "id", "modules", "page_routes" },
        "profile");
    PolicyProfile result;
    result.id = value.at("id").as_string();
    result.description = value.get("description", std::string());
    result.modules = parse_string_array(value, "modules");
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
    const auto routes = value.at("page_routes");
    if (!routes.is_array()) {
        invalid_config("profile page_routes must be an array");
    }
    std::unordered_set<std::string> route_ids;
    for (const auto& route_json : routes.as_array()) {
        auto route = parse_page_route(route_json);
        if (!route_ids.emplace(route.id).second) {
            invalid_config("profile contains duplicate page route: " + route.id);
        }
        page_routes->emplace_back(std::move(route));
    }
    return result;
}

TaskEventEffectKind parse_effect_kind(const std::string& value)
{
    if (value == "set") {
        return TaskEventEffectKind::Set;
    }
    if (value == "add") {
        return TaskEventEffectKind::Add;
    }
    if (value == "capture_int") {
        return TaskEventEffectKind::CaptureInteger;
    }
    invalid_config("unknown task event effect kind: " + value);
}

TaskEventEffect parse_task_effect(const json::value& value)
{
    check_keys(
        value,
        { "kind", "fact", "value", "source", "minimum", "maximum" },
        { "kind", "fact" },
        "task event effect");
    TaskEventEffect result;
    result.kind = parse_effect_kind(value.at("kind").as_string());
    result.fact = value.at("fact").as_string();
    if (const auto literal = value.find("value"); literal) {
        result.value = parse_fact_value(*literal);
    }
    result.source = value.get("source", std::string());
    result.minimum = value.get("minimum", std::numeric_limits<int>::min());
    result.maximum = value.get("maximum", std::numeric_limits<int>::max());
    if (result.fact.empty() || result.minimum > result.maximum) {
        invalid_config("task event effect has an invalid fact or numeric range");
    }
    if ((result.kind == TaskEventEffectKind::Set || result.kind == TaskEventEffectKind::Add) !=
        result.value.has_value()) {
        invalid_config("set/add effects require value and capture_int forbids value");
    }
    if (result.kind == TaskEventEffectKind::CaptureInteger) {
        if (result.source != "details.result.text") {
            invalid_config("capture_int source must be details.result.text");
        }
    }
    else if (!result.source.empty()) {
        invalid_config("only capture_int effects may specify source");
    }
    return result;
}

TaskEvent parse_task_event(const json::value& value)
{
    check_keys(
        value,
        { "task", "on", "effects", "outcome", "terminate", "termination_reason" },
        { "task", "on", "effects" },
        "task event");
    if (value.at("on").as_string() != "SubTaskCompleted") {
        invalid_config("task event on must be SubTaskCompleted");
    }
    TaskEvent result;
    result.task = value.at("task").as_string();
    result.outcome_code = value.get("outcome", std::string());
    result.terminate = value.get("terminate", false);
    result.termination_reason = value.get("termination_reason", std::string());
    if (result.task.empty() || !value.at("effects").is_array()) {
        invalid_config("task event task must be present and effects must be an array");
    }
    for (const auto& effect : value.at("effects").as_array()) {
        result.effects.emplace_back(parse_task_effect(effect));
    }
    if (result.terminate && (result.outcome_code.empty() || result.termination_reason.empty())) {
        invalid_config("terminating task event requires outcome and termination_reason");
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
        validate_condition(rule.target, facts, true);
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
        validate_condition(milestone.target, facts, true);
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
    const std::vector<PageRoute>& routes,
    const std::unordered_map<std::string, PolicyModule>& modules,
    const std::unordered_map<std::string, FactDefinition>& facts)
{
    ResolvedPolicy resolved;
    std::unordered_set<std::string> rule_ids;
    std::unordered_set<std::string> reserve_ids;
    std::unordered_set<std::string> milestone_ids;
    std::string error;
    for (const auto& module_id : profile.modules) {
        const auto module = modules.find(module_id);
        if (module == modules.end()) {
            invalid_config("profile references unknown module: " + module_id);
        }
        if (!append_unique(resolved.rules, module->second.rules, rule_ids, &error, "rule") ||
            !append_unique(resolved.reserves, module->second.reserves, reserve_ids, &error, "reserve") ||
            !append_unique(resolved.milestones, module->second.milestones, milestone_ids, &error, "milestone")) {
            invalid_config(error);
        }
    }
    for (const auto& milestone : resolved.milestones) {
        for (const auto& reserve : milestone.reserve_ids) {
            if (!reserve_ids.contains(reserve)) {
                invalid_config("milestone references unknown reserve: " + reserve);
            }
        }
        for (const auto& required : milestone.prerequisites) {
            if (!milestone_ids.contains(required)) {
                invalid_config("milestone requires unknown milestone: " + required);
            }
        }
        for (const auto& successor : milestone.successors) {
            if (!milestone_ids.contains(successor)) {
                invalid_config("milestone references unknown successor: " + successor);
            }
        }
    }
    validate_requirement_cycles(resolved.milestones);

    for (const auto& route : routes) {
        validate_condition(route.when, facts, false);
        if (Task.get(route.alias) == nullptr) {
            invalid_config("page route references unknown task alias: " + route.alias);
        }
        if (Task.get(route.task) == nullptr) {
            invalid_config("page route references unknown target task: " + route.task);
        }
    }
}

void validate_task_event(const TaskEvent& event, const std::unordered_map<std::string, FactDefinition>& facts)
{
    if (Task.get(event.task) == nullptr) {
        invalid_config("task event references unknown task: " + event.task);
    }
    for (const auto& effect : event.effects) {
        const auto definition = facts.find(effect.fact);
        if (definition == facts.end()) {
            invalid_config("task event references undeclared fact: " + effect.fact);
        }
        if (definition->second.scope == FactScope::Candidate) {
            invalid_config("task event cannot write candidate-scoped fact: " + effect.fact);
        }
        if (effect.kind == TaskEventEffectKind::CaptureInteger && definition->second.type != FactType::Integer) {
            invalid_config("capture_int effect requires an integer fact");
        }
        if (effect.kind == TaskEventEffectKind::Add &&
            (definition->second.type != FactType::Integer || !std::holds_alternative<std::int64_t>(*effect.value))) {
            invalid_config("add effect requires an integer fact and integer value");
        }
        if (effect.kind == TaskEventEffectKind::Set && !fact_value_matches(definition->second.type, *effect.value)) {
            invalid_config("set effect value type differs from fact declaration");
        }
    }
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

const std::vector<blackflow::PageRoute>*
    BlackFlowStrategyConfig::get_page_routes(const std::string& profile) const noexcept
{
    const auto found = m_page_routes.find(profile);
    return found == m_page_routes.end() ? nullptr : &found->second;
}

const blackflow::TaskEvent* BlackFlowStrategyConfig::get_task_event(const std::string& task) const noexcept
{
    const auto found = m_task_events.find(task);
    return found == m_task_events.end() ? nullptr : &found->second;
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
    result.failure_action = profile->failure_action;

    std::unordered_set<std::string> rule_ids;
    std::unordered_set<std::string> reserve_ids;
    std::unordered_set<std::string> milestone_ids;
    for (const auto& module_id : profile->modules) {
        const blackflow::PolicyModule* module = get_module(module_id);
        if (module == nullptr || !append_unique(result.rules, module->rules, rule_ids, error, "rule") ||
            !append_unique(result.reserves, module->reserves, reserve_ids, error, "reserve") ||
            !append_unique(result.milestones, module->milestones, milestone_ids, error, "milestone")) {
            if (module == nullptr && error != nullptr) {
                *error = "profile references unknown module: " + module_id;
            }
            return std::nullopt;
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
        { "schema_version", "resources", "facts", "modules", "profiles", "task_events" },
        { "schema_version", "resources", "facts", "modules", "profiles", "task_events" },
        "root");
    const int schema_version = json.at("schema_version").as_integer();
    if (schema_version != 2) {
        invalid_config("unsupported schema_version: " + std::to_string(schema_version));
    }
    for (const auto key : { "resources", "facts", "modules", "profiles", "task_events" }) {
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
    std::unordered_map<std::string, std::vector<blackflow::PageRoute>> page_routes;
    for (const auto& value : json.at("profiles").as_array()) {
        std::vector<blackflow::PageRoute> routes;
        auto profile = parse_profile(value, &routes);
        const std::string id = profile.id;
        if (!profiles.emplace(id, profile).second) {
            invalid_config("duplicate profile id: " + id);
        }
        page_routes.emplace(id, std::move(routes));
    }

    std::unordered_map<std::string, blackflow::TaskEvent> task_events;
    for (const auto& value : json.at("task_events").as_array()) {
        auto event = parse_task_event(value);
        validate_task_event(event, facts);
        if (!task_events.emplace(event.task, event).second) {
            invalid_config("duplicate task event: " + event.task);
        }
    }
    if (resources.empty() || facts.empty() || modules.empty() || profiles.empty()) {
        invalid_config("resources, facts, modules, and profiles must not be empty");
    }
    for (const auto& [id, profile] : profiles) {
        validate_profile_definition(profile, page_routes.at(id), modules, facts);
    }

    m_schema_version = schema_version;
    m_resources = std::move(resources);
    m_facts = std::move(facts);
    m_modules = std::move(modules);
    m_profiles = std::move(profiles);
    m_page_routes = std::move(page_routes);
    m_task_events = std::move(task_events);
    return true;
}
} // namespace asst
