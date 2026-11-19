#include "StrongholdProtocolConfig.h"

#include "Utils/Logger.hpp"

bool asst::StrongholdProtocolConfig::parse(const json::value& json)
{
    LogTraceFunction;

    // purchase.json 与 tiles.json 分别 load 本配置：按字段存在性独立刷新，
    // 不存在的字段保持原值，两次加载各自更新自己文件的内容
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
    if (!parse_string_array("priority", m_priority) || !parse_string_array("safe_keys", m_safe_keys) ||
        !parse_string_array("dps", m_dps) || !parse_string_array("healers", m_healers)) {
        return false;
    }

    // tiles.json 顶层即格子结构：safe/path 必填，rest/lift 可选
    if (!json.exists("safe") || !json.exists("path")) {
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
        !parse_point_array("rest", tiles.rest)) {
        return false;
    }
    if (const auto lift = json.find("lift"); lift && !parse_point(*lift, tiles.lift)) {
        LogError << "invalid tiles lift point";
        return false;
    }
    m_tiles = std::move(tiles);
    return true;
}
