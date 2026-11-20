#include "StrongholdProtocolConfig.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

int asst::StrongholdProtocolConfig::priority(const std::string& name, bool squad_only) const
{
    for (size_t i = 0; i < m_purchase.priority.size(); ++i) {
        const auto& [oper, squad] = m_purchase.priority[i];
        if (name == oper->name && (!squad_only || squad)) {
            return static_cast<int>(m_purchase.priority.size() - i);
        }
    }
    return 0;
}

asst::StrongholdProtocolConfig::Purchase::Oper
    asst::StrongholdProtocolConfig::season_oper(const std::string& name) const
{
    if (const auto it = m_purchase.forms.find(name); it != m_purchase.forms.end()) {
        return it->second;
    }
    if (const auto it = m_purchase.substitutes.find(name); it != m_purchase.substitutes.end()) {
        return it->second;
    }
    for (const auto& entry : m_purchase.priority) {
        if (entry.first->name == name) {
            return entry.first;
        }
    }
    return nullptr;
}

asst::battle::Role asst::StrongholdProtocolConfig::get_role(const std::string& name) const
{
    const auto roles = BattleData.get_roles(name, true);
    if (roles.size() == 1) {
        return *roles.begin();
    }
    if (const auto oper = season_oper(name); oper && roles.contains(oper->role)) {
        return oper->role;
    }
    return battle::Role::Unknown;
}

bool asst::StrongholdProtocolConfig::is_six_star(const std::string& name) const
{
    if (const auto oper = season_oper(name)) {
        return oper->rarity == 6;
    }
    auto opers = BattleData.find_opers(battle::Role::Unknown, name);
    std::erase_if(opers, [](const auto& oper) {
        return oper->role == battle::Role::Unknown || oper->role == battle::Role::Drone;
    });
    return !opers.empty() && std::ranges::all_of(opers, [](const auto& oper) { return oper->rarity == 6; });
}

bool asst::StrongholdProtocolConfig::parse_purchase(const json::value& json, bool custom)
{
    if (!json.is_object()) {
        LogError << __FUNCTION__ << "expected a purchase configuration object";
        return false;
    }
    const auto priority = json.find("priority");
    if (custom && !json.exists("forms") && !json.exists("substitutes") &&
        ((!priority && (json.exists("role_overrides") || json.exists("dps"))) ||
         (priority && priority->is_array() && !priority->as_array().empty() &&
          std::ranges::all_of(priority->as_array(), [](const auto& entry) { return entry.is_string(); })))) {
        LogWarn << __FUNCTION__
                << "ignoring legacy purchase_custom.json; use priority objects with id and optional squad,"
                   " and operator IDs in substitutes and forms";
        return true;
    }

    Purchase purchase = custom ? m_purchase : Purchase {};
    for (const std::string key : { "priority", "substitutes", "forms" }) {
        const auto entries = json.find(key);
        if (!entries) {
            continue;
        }
        if (!entries->is_array()) {
            LogError << __FUNCTION__ << "expected an array for" << key;
            return false;
        }
        if (key == "priority") {
            purchase.priority.clear();
        }
        else if (key == "substitutes") {
            purchase.substitutes.clear();
        }
        else {
            purchase.forms.clear();
        }
        std::unordered_set<std::string> names;
        for (const auto& entry : entries->as_array()) {
            const bool ordered = key == "priority";
            if (ordered ? !entry.is_object() || !entry.find<std::string>("id") ||
                              (entry.exists("squad") && !entry.find<bool>("squad"))
                        : !entry.is_string()) {
                LogError << __FUNCTION__ << "invalid entry in" << key << entry;
                return false;
            }
            const auto& id = ordered ? entry.at("id").as_string() : entry.as_string();
            const auto oper = BattleData.find_oper_by_id(id);
            const bool squad = ordered && entry.get("squad", false);
            if (!oper || oper->name.empty() || oper->role == battle::Role::Drone ||
                oper->role == battle::Role::Unknown || ((squad || key == "substitutes") && oper->rarity != 6) ||
                !names.emplace(oper->name).second) {
                LogError << __FUNCTION__ << "invalid or duplicate operator in" << key << id;
                return false;
            }
            if (ordered) {
                purchase.priority.emplace_back(oper, squad);
            }
            else if (key == "substitutes") {
                purchase.substitutes.emplace(oper->name, oper);
            }
            else {
                purchase.forms.emplace(oper->name, oper);
            }
        }
    }
    m_purchase = std::move(purchase);
    return true;
}

bool asst::StrongholdProtocolConfig::parse(const json::value& json)
{
    LogTraceFunction;

    const bool custom = m_path.filename() == utils::path("purchase_custom.json");
    if (custom || m_path.filename() == utils::path("purchase.json")) {
        return parse_purchase(json, custom);
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
    m_tiles = std::move(tiles);
    return true;
}
