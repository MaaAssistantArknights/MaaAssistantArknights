#include "StrongholdProtocolConfig.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

asst::battle::Role asst::StrongholdProtocolConfig::get_role(const std::string& name) const
{
    const auto roles = BattleData.get_roles(name, true);
    if (roles.size() == 1) {
        return *roles.begin();
    }
    const auto it = m_role_overrides.find(name);
    if (it != m_role_overrides.end() && roles.contains(it->second)) {
        return it->second;
    }
    return battle::Role::Unknown;
}

bool asst::StrongholdProtocolConfig::parse(const json::value& json)
{
    LogTraceFunction;

    auto parse_string_array = [&json](const std::string& key, std::vector<std::string>& out) {
        const auto arr = json.find(key);
        if (!arr) {
            return true;
        }
        if (!arr->is_array()) {
            LogError << "field is not an array:" << key;
            return false;
        }
        std::vector<std::string> parsed;
        for (const auto& item : arr->as_array()) {
            if (!item.is_string()) {
                LogError << "invalid string entry in" << key;
                return false;
            }
            parsed.emplace_back(item.as_string());
        }
        out = std::move(parsed);
        return true;
    };
    if (!parse_string_array("priority", m_priority) || !parse_string_array("dps", m_dps)) {
        return false;
    }
    if (const auto overrides = json.find("role_overrides")) {
        if (!overrides->is_object()) {
            LogError << __FUNCTION__ << "role_overrides must be an object";
            return false;
        }
        std::unordered_map<std::string, battle::Role> parsed;
        for (const auto& [name, value] : overrides->as_object()) {
            const auto role = value.is_string() ? battle::parse_role_type(value.as_string(), battle::Role::Unknown)
                                                : battle::Role::Unknown;
            if (role == battle::Role::Unknown) {
                LogError << __FUNCTION__ << "invalid operator role override for" << name;
                return false;
            }
            parsed.emplace(name, role);
        }
        m_role_overrides = std::move(parsed);
    }
    else if (m_path.filename() == utils::path("purchase.json")) {
        // 新赛季可以省略职业覆盖。格位文件和自定义配置则只更新其中列出的字段。
        m_role_overrides.clear();
    }

    if (!json.exists("safe") && !json.exists("path") && !json.exists("rear")) {
        return true;
    }
    if (!json.exists("rear")) {
        // 旧格表的 rest 混有安全格和堵口格，无法推断自定义坐标中哪一格是 C7R1。
        LogWarn << __FUNCTION__ << "legacy tile layout has no rear group; keeping the loaded layout";
        return m_path.filename() == utils::path("tiles_custom.json") && !m_tiles.rear.empty();
    }
    auto parse_point = [](const json::value& item, Point& out) {
        if (!item.is_array() || item.as_array().size() != 2 || !item.as_array()[0].is_number() ||
            !item.as_array()[1].is_number()) {
            return false;
        }
        out = Point { item.as_array()[0].as_integer(), item.as_array()[1].as_integer() };
        return true;
    };
    auto parse_point_array = [&json, &parse_point](const std::string& key, std::vector<Point>& out) {
        const auto arr = json.find(key);
        if (!arr) {
            return true;
        }
        if (!arr->is_array()) {
            LogError << "tiles field is not an array:" << key;
            return false;
        }
        for (const auto& item : arr->as_array()) {
            Point point;
            if (!parse_point(item, point)) {
                LogError << "invalid point in tiles field:" << key;
                return false;
            }
            out.emplace_back(point);
        }
        return true;
    };
    StrongholdTiles tiles;
    if (!parse_point_array("safe", tiles.safe) || !parse_point_array("path", tiles.path) ||
        !parse_point_array("rear", tiles.rear) || tiles.safe.empty() || tiles.path.empty() || tiles.rear.empty()) {
        return false;
    }
    if (const auto lift = json.find("lift"); lift && !parse_point(*lift, tiles.lift)) {
        LogError << "invalid tiles lift point";
        return false;
    }
    m_tiles = std::move(tiles);
    return true;
}
