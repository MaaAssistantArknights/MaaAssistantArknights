#include "ActivityNavigationConfig.h"

#include <regex>
#include <unordered_set>

#include "Config/Miscellaneous/TilePack.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

std::optional<asst::activity_navigation::Route> asst::ActivityNavigationConfig::find(const std::string& stage_id) const
{
    const auto it = m_routes.find(stage_id);
    if (it == m_routes.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<asst::activity_navigation::Route> asst::ActivityNavigationConfig::find_code(const std::string& code) const
{
    for (const auto& [id, route] : m_routes) {
        if (route.code == code) {
            return route;
        }
    }
    return std::nullopt;
}

bool asst::ActivityNavigationConfig::parse(const json::value& root)
{
    using activity_navigation::parse_time;
    using activity_navigation::Route;
    const auto fail = [] {
        LogError << "Invalid activity navigation configuration";
        return false;
    };
    const auto activities = root.find<json::array>("activities");
    if (root.find<int>("schema_version") != 1 || !activities) {
        return fail();
    }
    std::unordered_map<std::string, Route> parsed;
    std::unordered_set<std::string> ids;
    std::unordered_set<std::string> codes;
    const std::regex identifier("[A-Za-z0-9_-]{1,160}");
    const std::regex code_name("[A-Za-z0-9]+(-[A-Za-z0-9]+){1,3}");
    const std::regex partition_name("[a-z][a-z0-9_]{0,31}");
    const std::regex task_name("[A-Za-z][A-Za-z0-9_@-]{0,127}");
    for (const auto& activity : *activities) {
        auto id = activity.find<std::string>("id");
        auto enabled = activity.find<bool>("enabled");
        auto clients = activity.find<json::array>("clients");
        auto partitions = activity.find<json::object>("routes");
        auto stages = activity.find<json::object>("stages");
        if (!id || !std::regex_match(*id, identifier) || !ids.insert(*id).second || !enabled || !clients ||
            clients->empty() || !partitions || partitions->empty() || !stages || stages->empty()) {
            return fail();
        }
        std::vector<std::string> client_names;
        for (const auto& client : *clients) {
            if (!client.is_string()) {
                return fail();
            }
            auto name = client.as_string();
            if (name != "Official" && name != "Bilibili" && name != "YoStarEN" && name != "YoStarJP" &&
                name != "YoStarKR" && name != "txwy") {
                return fail();
            }
            if (std::find(client_names.begin(), client_names.end(), name) != client_names.end()) {
                return fail();
            }
            client_names.emplace_back(std::move(name));
        }
        std::unordered_map<std::string, Route> routes;
        for (const auto& [key, entry] : *partitions) {
            auto task = entry.find<std::string>("task");
            auto begin = entry.find<std::string>("opens_at");
            auto end = entry.find<std::string>("closes_at");
            if (!std::regex_match(key, partition_name) || !task || !std::regex_match(*task, task_name) || !begin ||
                !end) {
                return fail();
            }
            auto opens = parse_time(*begin), closes = parse_time(*end);
            if (!opens || !closes || *opens >= *closes) {
                return fail();
            }
            Route route;
            route.activity = *id;
            route.partition = key;
            route.task = *task;
            route.clients = client_names;
            route.enabled = *enabled;
            route.opens_at = *opens;
            route.closes_at = *closes;
            routes.emplace(key, std::move(route));
        }
        for (const auto& [stage_id, entry] : *stages) {
            auto code = entry.find<std::string>("code");
            auto partition = entry.find<std::string>("route");
            auto modes = entry.find<json::array>("modes");
            if (!stage_id.starts_with(*id + "_") || !std::regex_match(stage_id, identifier) || !code ||
                !std::regex_match(*code, code_name) || !codes.insert(*code).second || !partition ||
                !routes.contains(*partition) || !modes || modes->empty()) {
                return fail();
            }
            auto route = routes.at(*partition);
            route.stage_id = stage_id;
            route.code = *code;
            for (const auto& mode : *modes) {
                if (!mode.is_string()) {
                    return fail();
                }
                auto name = mode.as_string();
                if (name == "normal" && !route.normal) {
                    route.normal = true;
                }
                else if (name == "raid" && !route.raid) {
                    route.raid = true;
                }
                else {
                    return fail();
                }
            }
            if (!parsed.emplace(stage_id, std::move(route)).second) {
                return fail();
            }
        }
    }
    for (const auto& [id, route] : parsed) {
        const auto map = Tile.find(id);
        if (!map || map->first.code != route.code || !Task.get(route.task)) {
            return fail();
        }
    }
    // Replace only after validating the complete snapshot. An empty list explicitly disables all routes.
    m_routes = std::move(parsed);
    return true;
}
