#pragma once

#include <meojson/json.hpp>
#include <optional>
#include <set>
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
        std::vector<int> floor_window;
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
            if (!json.is_object()) {
                return false;
            }

            const json::object& obj = json.as_object();

            constexpr std::array<const char*, 9> allowed_keys = { "id",         "page_intent", "floor_window",
                                                                  "node_types", "event_names", "rank",
                                                                  "alias",      "task",        "completion_task" };
            for (const auto& kv : obj) {
                if (std::ranges::find(allowed_keys, kv.first) == allowed_keys.end()) {
                    return false;
                }
            }
            const auto check_field = [&]<typename T>(const char* key, bool required = true) -> bool {
                const auto it = obj.find<T>(key);
                if (it == std::nullopt) {
                    return !required;
                }
                return true;
            };

#define field_check(key, required) check_field.template operator()<decltype(key)>(#key, required)
            return field_check(id, true) && field_check(page_intent, true) && field_check(floor_window, false) &&
                   field_check(node_types, false) && field_check(event_names, false) && field_check(rank, false) &&
                   field_check(alias, true) && field_check(task, true) && field_check(completion_task, true);
#undef field_check
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

    [[nodiscard]] bool
        parse_node_types(const std::vector<std::string>& value, std::vector<blackflow::NodeType>& out) const;

    bool verify_non_empty(const std::vector<std::string>& value, const std::string& key) const;
    bool sort_and_check_unique(std::vector<std::string>& value, const std::string& key) const;
    bool parse_floor_window(const std::vector<int>& value, int& low, int& high) const;
    bool verify_task(const std::string& task, const std::string& field) const;

    int m_schema_version = 0;
    std::vector<blackflow::NodeExecutionRoute> m_routes;
    std::unordered_map<std::string, blackflow::NodeTaskResult> m_task_results;
    std::vector<std::string> m_preview_names;
    std::unordered_map<std::string, blackflow::NodeType> m_preview_name_types;
};

inline static auto& BlackFlowNodeExecution = BlackFlowNodeExecutionConfig::get_instance();
} // namespace asst
