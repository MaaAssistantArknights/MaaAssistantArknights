#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <meojson/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Config/AbstractConfig.h"
#include "Task/Roguelike/BlackFlow/BlackFlowNodeExecutionTypes.h"

namespace json::_reflection
{
template <>
struct enum_name_storage<asst::blackflow::NodeType>
{
    static constexpr int min = 0;
    static constexpr int max = 21;
    static constexpr int range = max - min + 1;
    static constexpr std::array<std::string_view, range> names {
        "unclassified", "battle_elite", "battle_normal", "battle_savage",  "duel",     "door",
        "employ",       "expedition",   "hide_battle",   "hide_invisible", "incident", "light",
        "portal",       "rest",         "sacrifice",     "scrap_shop",     "shop",     "wish",
        "empty",        "evacuate",     "final",         "battle_boss",
    };
};
}

namespace asst
{
class BlackFlowNodeExecutionConfig final :
    public MAA_NS::SingletonHolder<BlackFlowNodeExecutionConfig>,
    public AbstractConfig
{
private:
    struct NodeExecutionRouteDto
    {
        std::string id;
        std::string page_intent;
        std::array<int, 2> floor_window = { 1, std::numeric_limits<int>::max() };
        std::vector<std::string> node_types;
        std::vector<std::string> event_names;
        int rank;
        std::string alias;
        std::string task;
        std::string completion_task;

        MEO_TOJSON(
            id,
            page_intent,
            MEO_OPT floor_window,
            MEO_OPT node_types,
            MEO_OPT event_names,
            MEO_OPT rank,
            alias,
            task,
            completion_task);

        bool check_json(const json::value& json) const
        {
            static constexpr std::array<const char*, 9> allowed_keys = {
                "id",   "page_intent", "floor_window", "node_types",     "event_names",
                "rank", "alias",       "task",         "completion_task"
            };

            if (!json.is_object()) {
                return false;
            }
            const auto& obj = json.as_object();
            for (const auto& kv : obj) {
                if (std::ranges::find(allowed_keys, kv.first) == allowed_keys.end()) {
                    return false;
                }
            }

            bool ret = true;
            const auto check_field =
                [&]<typename T>(const char* key, const T&, bool required = true) -> std::optional<T> {
                const auto& itoa = json.find_value(key);
                if (!itoa) {
                    ret = ret && !required;
                }
                else if (!itoa->is<T>()) {
                    ret = false;
                }
                else {
                    return itoa->as<T>();
                }
                return std::nullopt;
            };
#define field(key) [[maybe_unused]] const auto& key##_opt = check_field(#key, key, true)
#define field_opt(key) [[maybe_unused]] const auto& key##_opt = check_field(#key, key, false)
            field(id);
            field(page_intent);
            field_opt(floor_window);
            field_opt(node_types);
            field_opt(event_names);
            field_opt(rank);
            field(alias);
            field(task);
            field(completion_task);
#undef field
#undef field_opt
            if (!ret) {
                return false;
            }
            if (id_opt.value().empty()) {
                LogError << __FUNCTION__ << "id must be non-empty";
                ret = false;
            }
            if (floor_window_opt &&
                (floor_window_opt->at(0) < 1 || (floor_window_opt->at(0) > floor_window_opt->at(1)))) {
                LogError << __FUNCTION__ << "floor_window must be a positive, ascending range";
                ret = false;
            }
            return ret;
        };

        MEO_FROMJSON(
            id,
            page_intent,
            MEO_OPT floor_window,
            MEO_OPT node_types,
            MEO_OPT event_names,
            MEO_OPT rank,
            alias,
            task,
            completion_task);
    };

    struct NodeStateUpdateDto
    {
        std::optional<blackflow::NodeProgress> progress;
        std::optional<blackflow::NodeType> actual_type;
        std::optional<std::string> actual_name;
        std::string actual_name_source;
        std::optional<bool> identity_revealed;
        std::optional<bool> repeatable;
        std::optional<bool> becomes_empty;
        MEO_TOJSON(
            MEO_OPT actual_name_source,
            MEO_OPT progress,
            MEO_OPT actual_type,
            MEO_OPT actual_name,
            MEO_OPT identity_revealed,
            MEO_OPT repeatable,
            MEO_OPT becomes_empty);
        MEO_FROMJSON(
            MEO_OPT actual_name_source,
            MEO_OPT progress,
            MEO_OPT actual_type,
            MEO_OPT actual_name,
            MEO_OPT identity_revealed,
            MEO_OPT repeatable,
            MEO_OPT becomes_empty);

        bool check_json(const json::value& json) const
        {
            static constexpr std::array<const char*, 7> allowed_keys = { "actual_name_source", "progress",
                                                                         "actual_type",        "actual_name",
                                                                         "identity_revealed",  "repeatable",
                                                                         "becomes_empty" };

            if (!json.is_object()) {
                return false;
            }
            const auto& obj = json.as_object();
            for (const auto& kv : obj) {
                if (std::ranges::find(allowed_keys, kv.first) == allowed_keys.end()) {
                    return false;
                }
            }

            bool ret = true;
            const auto check_field =
                [&]<typename T>(const char* key, const T&, bool required = true) -> std::optional<T> {
                const auto& itoa = json.find_value(key);
                if (!itoa) {
                    ret = ret && !required;
                }
                else if (!itoa->is<T>()) {
                    ret = false;
                }
                else {
                    return itoa->as<T>();
                }
                return std::nullopt;
            };
#define field_opt(key) [[maybe_unused]] const auto& key##_opt = check_field(#key, key, false)
            field_opt(actual_name_source);
            field_opt(progress);
            field_opt(actual_type);
            field_opt(actual_name);
            field_opt(identity_revealed);
            field_opt(repeatable);
            field_opt(becomes_empty);
#undef field_opt
            if (!ret) {
                return false;
            }
            if (actual_name_opt && actual_name_source_opt) {
                LogError << __FUNCTION__ << "node update cannot define actual_name and actual_name_source together";
                return false;
            }
            if (actual_name_opt && actual_name_opt->value().empty()) {
                LogError << __FUNCTION__ << "node update actual_name must not be empty";
                return false;
            }
            return ret;
        }
    };

    struct NodeStrategySignalDto
    {
        blackflow::NodeSignalKind kind = blackflow::NodeSignalKind::Set;
        std::string fact;
        std::optional<std::variant<bool, std::int64_t, std::string, std::vector<std::string>>> value;
        std::string source;
        int minimum = std::numeric_limits<int>::min();
        int maximum = std::numeric_limits<int>::max();

        MEO_TOJSON(kind, fact, MEO_OPT value, MEO_OPT source, MEO_OPT minimum, MEO_OPT maximum);
        MEO_FROMJSON(kind, fact, MEO_OPT value, MEO_OPT source, MEO_OPT minimum, MEO_OPT maximum);

        bool check_json(const json::value& json) const
        {
            static constexpr std::array<const char*, 6> allowed_keys = { "kind",   "fact",    "value",
                                                                         "source", "minimum", "maximum" };
            if (!json.is_object()) {
                return false;
            }
            const auto& obj = json.as_object();
            for (const auto& kv : obj) {
                if (std::ranges::find(allowed_keys, kv.first) == allowed_keys.end()) {
                    return false;
                }
            }
            bool ret = true;
            const auto check_field =
                [&]<typename T>(const char* key, const T&, bool required = true) -> std::optional<T> {
                const auto& itoa = json.find_value(key);
                if (!itoa) {
                    ret = ret && !required;
                }
                else if (!itoa->is<T>()) {
                    ret = false;
                }
                else {
                    return itoa->as<T>();
                }
                return std::nullopt;
            };
#define field(key) [[maybe_unused]] const auto& key##_opt = check_field(#key, key, true)
#define field_opt(key) [[maybe_unused]] const auto& key##_opt = check_field(#key, key, false)
            field(kind);
            field(fact);
            field_opt(value);
            field_opt(source);
            field_opt(minimum);
            field_opt(maximum);
#undef field
#undef field_opt
            if (!ret) {
                return false;
            }
            if (fact_opt.value().empty()) {
                LogError << __FUNCTION__ << "fact must be non-empty";
                return false;
            }
            if (kind_opt.value() == blackflow::NodeSignalKind::Set) {
                if (!value_opt || source_opt || minimum_opt || maximum_opt) {
                    LogError << __FUNCTION__
                             << "set signal requires value and does not accept source or integer bounds";
                    return false;
                }
            }
            else if (*kind_opt == blackflow::NodeSignalKind::Add) {
                if (!value_opt || !value_opt.value().has_value() || source_opt || minimum_opt || maximum_opt) {
                    LogError << __FUNCTION__
                             << "add signal requires an integer value and does not accept source or integer bounds";
                    return false;
                }
                if (!std::holds_alternative<std::int64_t>(**value_opt)) {
                    LogError << __FUNCTION__ << "add signal value must be an integer";
                    return false;
                }
            }
            else {
                if (value_opt || !source_opt) {
                    LogError << __FUNCTION__ << "capture_integer signal requires source and does not accept value";
                    return false;
                }
                if (source_opt && *source_opt != "details.result.text") {
                    LogError << __FUNCTION__ << "capture_integer signal source must be details.result.text";
                    return false;
                }
                if (minimum_opt && maximum_opt && *minimum_opt > *maximum_opt) {
                    LogError << __FUNCTION__ << "capture_integer signal minimum exceeds maximum";
                    return false;
                }
            }
            return true;
        }
    };

    struct NodeTaskResultDto
    {
        std::string task;
        blackflow::NodeTaskResultKind kind = blackflow::NodeTaskResultKind::Intermediate;
        NodeStateUpdateDto node;
        std::vector<NodeStrategySignalDto> signals;
        std::string outcome_code;
        std::string termination_reason;
        bool terminate = false;
        bool succeeded = false;
        bool redispatch = false;

        MEO_TOJSON(
            task,
            MEO_KEY("result_kind") kind,
            MEO_OPT node,
            MEO_OPT signals,
            MEO_OPT MEO_KEY("outcome") outcome_code,
            MEO_OPT terminate,
            MEO_OPT succeeded,
            MEO_OPT redispatch,
            MEO_OPT termination_reason);

        MEO_FROMJSON(
            task,
            MEO_KEY("result_kind") kind,
            MEO_OPT node,
            MEO_OPT signals,
            MEO_OPT MEO_KEY("outcome") outcome_code,
            MEO_OPT terminate,
            MEO_OPT succeeded,
            MEO_OPT redispatch,
            MEO_OPT termination_reason);

        bool check_json(const json::value& json) const
        {
            static constexpr std::array<const char*, 9> allowed_keys = {
                "task",      "result_kind", "node",       "signals",           "outcome",
                "terminate", "succeeded",   "redispatch", "termination_reason"
            };

            if (!json.is_object()) {
                return false;
            }
            const auto& obj = json.as_object();
            for (const auto& kv : obj) {
                if (std::ranges::find(allowed_keys, kv.first) == allowed_keys.end()) {
                    return false;
                }
            }

            bool ret = true;
            const auto check_field =
                [&]<typename T>(const char* key, const T&, bool required = true) -> std::optional<T> {
                const auto& itoa = json.find_value(key);
                if (!itoa) {
                    ret = ret && !required;
                }
                else if (!itoa->is<T>()) {
                    ret = false;
                }
                else {
                    return itoa->as<T>();
                }
                return std::nullopt;
            };
#define field(key) [[maybe_unused]] const auto& key##_opt = check_field(#key, key, true)
#define field_opt(key) [[maybe_unused]] const auto& key##_opt = check_field(#key, key, false)
            field(task);
            const auto& kind_opt = check_field("result_kind", kind, true);
            field_opt(node);
            field_opt(signals);
            const auto& outcome_code_opt = check_field("outcome", outcome_code, false);
            field_opt(terminate);
            field_opt(succeeded);
            field_opt(redispatch);
            field_opt(termination_reason);
#undef field
#undef field_opt
            if (!ret) {
                return false;
            }
            if (terminate_opt.value_or(false) != succeeded_opt.has_value()) {
                LogError << __FUNCTION__ << "terminating value:" << terminate_opt.value_or(false)
                         << "is not match with existence of succeeded:" << succeeded_opt.has_value();
                return false;
            }
            if (terminate_opt.value_or(false)) {
                if (outcome_code_opt.value_or(std::string()).empty() ||
                    termination_reason_opt.value_or(std::string()).empty()) {
                    LogError << __FUNCTION__ << "terminating task result requires outcome and termination_reason";
                    return false;
                }
                if (kind_opt && *kind_opt != blackflow::NodeTaskResultKind::PageCompleted) {
                    LogError << __FUNCTION__ << "terminating task result must complete its page";
                    return false;
                }
            }
            else if (outcome_code_opt || termination_reason_opt) {
                LogError << __FUNCTION__ << "non-terminating task result must not define outcome or termination_reason";
                return false;
            }
            if (kind_opt && *kind_opt == blackflow::NodeTaskResultKind::PageCompleted &&
                redispatch_opt.value_or(false)) {
                LogError << __FUNCTION__ << "page-completed task result cannot request redispatch";
                return false;
            }
            return true;
        };
    };

    struct NodePreviewName
    {
        std::string name;
        std::string node_type;
        MEO_JSONIZATION(MEO_KEY("text") name, node_type);
    };

public:
    virtual ~BlackFlowNodeExecutionConfig() override = default;

    [[nodiscard]] const std::optional<std::reference_wrapper<const blackflow::NodeExecutionRoute>>
        resolve_route(const blackflow::NodeExecutionContext& context) const noexcept;
    [[nodiscard]] const std::optional<std::reference_wrapper<const blackflow::NodeTaskResult>>
        get_task_result(const std::string& task) const noexcept;
    [[nodiscard]] std::optional<blackflow::NodeType> preview_node_type(const std::string& text) const noexcept;

    [[nodiscard]] const std::vector<blackflow::NodeExecutionRoute>& routes() const noexcept { return m_routes; }

    [[nodiscard]] const std::vector<std::string>& preview_names() const noexcept { return m_preview_names; }

    [[nodiscard]] int schema_version() const noexcept { return m_schema_version; }

    bool parse_for_test(const json::value& json, std::string* error = nullptr);

private:
    virtual bool parse(const json::value& json) override;

    bool parse_route(
        const json::value& json,
        std::vector<blackflow::NodeExecutionRoute>& routes,
        std::unordered_set<std::string>& route_ids) const;

    bool parse_task_results(
        const json::value& json,
        std::unordered_map<std::string, blackflow::NodeTaskResult>& task_results) const;

    bool parse_preview_name(
        const json::value& json,
        std::vector<std::string>& preview_names,
        std::unordered_map<std::string, blackflow::NodeType>& preview_name_types) const;

    bool verify_page_intent(std::string_view value) const;
    bool parse_node_types(const std::vector<std::string>& value, std::vector<blackflow::NodeType>& out) const;
    bool verify_non_empty(const std::vector<std::string>& value, const std::string& key) const;
    bool sort_and_check_unique(std::vector<std::string>& value, const std::string& key) const;
    bool verify_task(const std::string& task, const std::string& field) const;

    int m_schema_version = 0;
    std::vector<blackflow::NodeExecutionRoute> m_routes;
    std::unordered_map<std::string, blackflow::NodeTaskResult> m_task_results;
    std::vector<std::string> m_preview_names;
    std::unordered_map<std::string, blackflow::NodeType> m_preview_name_types;
};

inline static auto& BlackFlowNodeExecution = BlackFlowNodeExecutionConfig::get_instance();
} // namespace asst
