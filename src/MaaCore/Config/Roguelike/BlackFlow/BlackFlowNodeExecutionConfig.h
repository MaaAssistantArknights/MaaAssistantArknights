#pragma once

#include <algorithm>
#include <array>
#include <limits>
#include <meojson/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Config/AbstractConfig.h"
#include "Task/Roguelike/BlackFlow/BlackFlowNodeExecutionTypes.h"

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

    struct NodePreviewName
    {
        std::string name;
        std::string node_type;
        MEO_JSONIZATION(MEO_KEY("text") name, node_type);
    };

public:
    virtual ~BlackFlowNodeExecutionConfig() override = default;

    [[nodiscard]] const blackflow::NodeExecutionRoute*
        resolve_route(const blackflow::NodeExecutionContext& context) const noexcept;
    [[nodiscard]] const blackflow::NodeTaskResult* get_task_result(const std::string& task) const noexcept;
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
