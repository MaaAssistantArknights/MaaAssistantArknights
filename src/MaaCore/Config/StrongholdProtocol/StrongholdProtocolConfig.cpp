#include "StrongholdProtocolConfig.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/TilePack.h"

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include <Arknights-Tile-Pos/TileCalc2.hpp>
MAA_SUPPRESS_CV_WARNINGS_END
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
    const auto byte_ok = [&](int value) {
        return in_range(value, 0, 255);
    };
    const auto [top, bottom] = scrollbar.track_y;
    const auto [pad_x, pad_y] = scrollbar.scan_padding;
    const auto& view = frame.viewport;
    if (!byte_ok(portrait_max_saturation) || !byte_ok(portrait_min_value) || !byte_ok(scrollbar.max_saturation) ||
        !byte_ok(scrollbar.min_value) || !in_range(scrollbar.min_fill, 0., 1.) ||
        !in_range(pad_x, 0, WindowWidthDefault) || !in_range(pad_y, 0, WindowHeightDefault) || top < pad_y ||
        bottom <= top || bottom >= WindowHeightDefault - pad_y || !in_range(scrollbar.min_height, 1, bottom - top) ||
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
        columns.empty() || scrollbar.columns.empty()) {
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
           std::ranges::all_of(columns, [&](int x) { return in_range(x, 0, WindowWidthDefault - 1); });
}

bool asst::StrongholdTierBadgeConfig::check_json(const json::value& json) const
{
    StrongholdTierBadgeConfig cfg;
    if (!json.is_object() || !cfg.from_json(json)) {
        return false;
    }
    for (const auto& [lo, hi] : cfg.hsv) {
        for (size_t channel = 0; channel < lo.size(); ++channel) {
            if (lo[channel] < 0 || lo[channel] > hi[channel] || hi[channel] > (channel == 0 ? 179 : 255)) {
                return false;
            }
        }
    }
    const auto ordered_fraction = [](const auto& bounds) {
        return 0 <= bounds[0] && bounds[0] < bounds[1] && bounds[1] <= 1;
    };
    return std::ranges::all_of(cfg.inset, [](double margin) { return margin >= 0 && margin < 0.5; }) &&
           ordered_fraction(cfg.glyph_height) && ordered_fraction(cfg.stroke_rows) &&
           ordered_fraction(cfg.foreground_fraction) && cfg.max_center_offset >= 0 && cfg.max_center_offset < 0.5 &&
           cfg.max_stroke_width > 0 && cfg.max_stroke_width <= 1 && cfg.max_stroke_drift >= 0 &&
           cfg.max_stroke_drift <= 1 && cfg.min_convergence > 0 && cfg.min_convergence < 1 &&
           cfg.min_color_fraction > 0.5 && cfg.min_color_fraction <= 1 && cfg.min_contrast > 0 &&
           cfg.min_contrast <= 255 && cfg.max_background_value >= 0 && cfg.max_background_value <= 255;
}

bool asst::StrongholdProtocolConfig::parse(const json::value& json)
{
    LogTraceFunction;

    if (m_path.filename() == utils::path("tier_badges.json") ||
        m_path.filename() == utils::path("tier_badges_custom.json")) {
        if (!json.is<StrongholdTierBadgeConfig>()) {
            LogError << __FUNCTION__ << "invalid tier badge configuration";
            return false;
        }
        m_tier_badges = json.as<StrongholdTierBadgeConfig>();
        return true;
    }

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
    if (!parse_string_array("bonds", m_bonds)) {
        return false;
    }

    // stronghold_protocol_data.json 中的赛季干员 ID、盟约和等阶。
    if (const auto operators = json.find("operators")) {
        if (!operators->is_object()) {
            LogError << "operators field is not an object";
            return false;
        }
        std::unordered_map<std::string, StrongholdOperInfo> parsed;
        for (const auto& [name, info] : operators->as_object()) {
            StrongholdOperInfo oper;
            oper.id = info.get("id", std::string {});
            if (const auto bonds = info.find("bonds"); bonds && bonds->is_array()) {
                for (const auto& bond : bonds->as_array()) {
                    if (!bond.is_string()) {
                        LogError << "invalid bond for operator:" << name;
                        return false;
                    }
                    oper.bonds.emplace_back(bond.as_string());
                }
            }
            if (const auto level = info.find("level"); level && level->is_number()) {
                oper.level = level->as_integer();
            }
            parsed.emplace(name, std::move(oper));
        }
        m_operators = std::move(parsed);
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
        !parse_point_array("rear", tiles.rear) || !parse_point_array("melee", tiles.melee) ||
        !parse_point_array("ranged", tiles.ranged) || !parse_point_array("medic", tiles.medic) || tiles.safe.empty() ||
        tiles.path.empty() || tiles.rear.empty()) {
        return false;
    }
    if (const auto stage = json.find<std::string>("covenantStage")) {
        const auto level = TilePack::find_level(*stage);
        if (!level || level->view.size() != 2) {
            LogError << __FUNCTION__ << "covenant map could not be loaded:" << *stage;
            return false;
        }

        const auto project = [&](std::vector<Point>& points, bool deployment) {
            using namespace Map::TileCalc2;
            // 普通与部署视角分别使用校准后的相机偏移、俯仰角和视野角。
            const vec3d offset = deployment ? vec3d { 0.0163, -0.6779, 0.1217 } : vec3d { 0.0039, -2.2213, -1.1865 };
            const auto matrix = camera_matrix_from_trans(
                camera_pos(*level, deployment) + offset,
                { 0, (deployment ? 32.4822 : 34.2279) * degree, 0 },
                static_cast<double>(WindowHeightDefault) / WindowWidthDefault,
                (deployment ? 19.1447 : 17.0441) * degree);
            for (auto& point : points) {
                if (point.x < 0 || point.x >= level->get_width() || point.y < 0 || point.y >= level->get_height()) {
                    LogError << __FUNCTION__ << "invalid covenant map tile" << point;
                    return false;
                }
                auto world = get_tile_world_pos(*level, point.y, point.x);
                world[2] = 0;
                const auto screen = matrix * cv::Point3d(world);
                point = {
                    static_cast<int>(std::round((1 + screen(0) / screen(3)) * WindowWidthDefault / 2)),
                    static_cast<int>(std::round((1 - screen(1) / screen(3)) * WindowHeightDefault / 2)),
                };
            }
            return true;
        };
        for (auto [key, points] :
             { std::pair { "melee", &tiles.melee }, { "ranged", &tiles.ranged }, { "medic", &tiles.medic } }) {
            if (points->empty() && !parse_point_array(key, *points)) {
                return false;
            }
            if (points->empty() || !project(*points, true)) {
                return false;
            }
        }
        // 按地图中的地块类型定位待部署席和装备席，席位顺序从右往左。
        for (int row = 0; row < level->get_height(); ++row) {
            for (int col = level->get_width() - 1; col >= 0; --col) {
                const auto tile = level->get_item(row, col);
                if (tile.tileKey == "tile_achand") {
                    tiles.bench.emplace_back(col, row);
                }
                else if (tile.tileKey == "tile_acbag") {
                    tiles.equipment.emplace_back(col, row);
                }
            }
        }
        if (tiles.bench.empty() || tiles.equipment.empty() || !project(tiles.bench, false) ||
            !project(tiles.equipment, false)) {
            LogError << __FUNCTION__ << "invalid covenant bench or equipment slots";
            return false;
        }
    }
    else {
        // 自定义格表可以覆盖部署坐标，待部署席和装备席沿用内置地图。
        if (tiles.melee.empty()) {
            tiles.melee = m_tiles.melee;
        }
        if (tiles.ranged.empty()) {
            tiles.ranged = m_tiles.ranged;
        }
        if (tiles.medic.empty()) {
            tiles.medic = m_tiles.medic;
        }
        tiles.bench = m_tiles.bench;
        tiles.equipment = m_tiles.equipment;
    }
    m_tiles = std::move(tiles);
    return true;
}
