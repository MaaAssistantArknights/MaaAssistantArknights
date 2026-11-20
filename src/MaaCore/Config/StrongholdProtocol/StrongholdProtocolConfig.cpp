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
    Purchase purchase = custom ? m_purchase : Purchase {};
    for (const std::string key : { "priority", "substitutes", "forms", "squad_blacklist" }) {
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
        else if (key == "forms") {
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
            else if (key == "forms") {
                purchase.forms.emplace(oper->name, oper);
            }
        }
        if (key == "squad_blacklist") {
            purchase.squad_blacklist = std::move(names);
        }
    }
    m_purchase = std::move(purchase);
    return true;
}

bool asst::StrongholdStrategyGrid::valid() const
{
    const auto in_range = [](auto value, auto low, auto high) {
        return low <= value && value <= high;
    };
    const auto hue_ok = [&](const auto& hue) {
        return in_range(hue[0], 0, hue[1]) && hue[1] <= 180;
    };
    const auto byte_ok = [&](int value) {
        return in_range(value, 0, 255);
    };
    const auto badge_ok = [&](const Badge& badge) {
        return hue_ok(badge.hue) && byte_ok(badge.min_saturation) && byte_ok(badge.min_value) &&
               in_range(badge.min_ratio, 0., 1.);
    };
    const auto [top, bottom] = scrollbar.track_y;
    const auto [pad_x, pad_y] = scrollbar.scan_padding;
    const auto& view = frame.viewport;
    if (!badge_ok(gold_badge) || !badge_ok(color_badge) || !in_range(color_badge_max_gold_ratio, 0., 1.) ||
        !byte_ok(marker_min_saturation) || !byte_ok(marker_min_value) || !byte_ok(portrait_max_saturation) ||
        !byte_ok(portrait_min_value) || !byte_ok(scrollbar.max_saturation) || !byte_ok(scrollbar.min_value) ||
        !in_range(scrollbar.min_fill, 0., 1.) || !in_range(pad_x, 0, WindowWidthDefault) ||
        !in_range(pad_y, 0, WindowHeightDefault) || top < pad_y || bottom <= top ||
        bottom >= WindowHeightDefault - pad_y || !in_range(scrollbar.min_height, 1, bottom - top) ||
        !in_range(scrollbar.thumb_width[0], 1, scrollbar.thumb_width[1]) || scrollbar.thumb_width[1] > 2 * pad_x + 1 ||
        !in_range(view.width, 1, WindowWidthDefault) || !in_range(view.height, 1, WindowHeightDefault) ||
        !in_range(view.y, 0, WindowHeightDefault - view.height) || !in_range(view.x, -WindowWidthDefault, 0) ||
        !in_range(frame.column_step, 1, WindowWidthDefault) || !in_range(frame.right_edge, 1, WindowWidthDefault) ||
        !in_range(frame.inner_columns[0], 0, frame.right_edge) ||
        !in_range(frame.inner_columns[1], 0, frame.right_edge) || !in_range(frame.height[1], 1, view.height) ||
        !in_range(frame.height[0], 1, frame.height[1]) || !in_range(frame.min_partial_height, 1, frame.height[0]) ||
        !in_range(frame.center_from_bottom_left[0], 0, frame.column_step) ||
        !in_range(frame.center_from_bottom_left[1], -frame.height[1], 0) ||
        !in_range(frame.candidate_left, -WindowWidthDefault, 0) ||
        !in_range(frame.candidate_y[0], 0, frame.candidate_y[1]) || frame.candidate_y[1] > WindowHeightDefault ||
        columns.empty() || scrollbar.columns.empty() || markers.empty()) {
        return false;
    }
    // 滑块中心允许偏一像素。视口和最外侧卡框的取样列都必须留在画面内。
    const int last_column = (view.width - 1) / frame.column_step * frame.column_step;
    return std::ranges::all_of(
               scrollbar.columns,
               [&](int x) {
                   if (!in_range(x, pad_x, WindowWidthDefault - pad_x - 1)) {
                       return false;
                   }
                   const int left = x + view.x;
                   return left - 1 >= 4 &&
                          left + 1 + std::max(view.width, last_column + frame.right_edge + 4) <= WindowWidthDefault;
               }) &&
           std::ranges::all_of(columns, [&](int x) { return in_range(x, 0, WindowWidthDefault - 1); }) &&
           std::ranges::all_of(markers, [&](const Marker& marker) {
               return hue_ok(marker.hue) &&
                      in_range(marker.center_offset[0], -WindowWidthDefault, WindowWidthDefault) &&
                      in_range(marker.center_offset[1], -WindowHeightDefault, WindowHeightDefault);
           });
}

bool asst::StrongholdProtocolConfig::parse(const json::value& json)
{
    LogTraceFunction;

    const bool custom = m_path.filename() == utils::path("purchase_custom.json");
    if (custom || m_path.filename() == utils::path("purchase.json")) {
        return parse_purchase(json, custom);
    }

    if (m_path.filename() == utils::path("strategy_grid.json") ||
        m_path.filename() == utils::path("strategy_grid_custom.json")) {
        if (!json.is<StrongholdStrategyGrid>()) {
            LogError << __FUNCTION__ << "invalid strategy grid configuration";
            return false;
        }
        m_strategy_grid = json.as<StrongholdStrategyGrid>();
        return true;
    }

    if (!json.exists("safe") && !json.exists("path") && !json.exists("rear")) {
        return true;
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
