#include "TilePack.h"

#include "MaaUtils/Conf.h"
#include "meojson/json.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <future>
#include <list>
#include <mutex>
#include <numeric>
#include <optional>
#include <queue>

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include <Arknights-Tile-Pos/TileCalc2.hpp>
MAA_SUPPRESS_CV_WARNINGS_END

#include "Utils/Logger.hpp"

bool asst::TilePack::parse(const json::value& json)
{
    LogTraceFunction;

    auto dir = m_path.parent_path();
    for (const auto& [_, summary] : json.as_object()) {
        LevelKey level_key {
            .stageId = summary.at("stageId").as_string(),
            .code = summary.at("code").as_string(),
            .levelId = summary.at("levelId").as_string(),
            .name = summary.get("name", "UnknownLevelName"),
        };
        auto filepath = dir / utils::path(summary.at("filename").as_string());
        // if (!std::filesystem::exists(filepath)) {
        //     Log.error("file not exists", filepath);
        //     return false;
        // }
        m_summarize.emplace_back(std::move(level_key), std::move(filepath));
    }
    return true;
}

bool proc_data(
    std::unordered_map<asst::Point, asst::TilePack::TileInfo>& dst,
    asst::Point loc,
    cv::Point cv_p,
    const Map::Tile& tile)
{
    using namespace asst;
    using TileKey = asst::TilePack::TileKey;
    using TileInfo = asst::TilePack::TileInfo;

    static const std::unordered_map<std::string, TileKey> TileKeyMapping = {
        { "tile_forbidden", TileKey::Forbidden },
        { "tile_wall", TileKey::Wall },
        { "tile_road", TileKey::Road },
        { "tile_end", TileKey::Home },
        { "tile_start", TileKey::EnemyHome },
        { "tile_green", TileKey::Green },
        { "tile_flystart", TileKey::Airport },
        { "tile_floor", TileKey::Floor },
        { "tile_hole", TileKey::Hole },
        { "tile_telin", TileKey::Telin },
        { "tile_telout", TileKey::Telout },
        { "tile_grass", TileKey::Grass },
        { "tile_deepsea", TileKey::DeepSea },
        { "tile_deepwater", TileKey::DeepSea },
        { "tile_volcano", TileKey::Volcano },
        { "tile_healing", TileKey::Healing },
        { "tile_fence", TileKey::Fence /* 疑似弃用, 改为 tile_fence_bound */ },
        { "tile_fence_bound", TileKey::Fence },
        { "tile_infection", TileKey::Infection },
    };

    auto key = TileKey::Invalid;
    if (auto iter = TileKeyMapping.find(tile.tileKey); iter != TileKeyMapping.cend()) {
        key = iter->second;
    }
    else {
        key = TileKey::Invalid;
        LogWarn << "Unknown tile type" << loc << ":" << tile.tileKey;
    }

    dst.emplace(
        loc,
        TileInfo { static_cast<battle::LocationType>(tile.buildableType),
                   static_cast<TilePack::HeightType>(tile.heightType),
                   key,
                   Point(cv_p.x, cv_p.y),
                   loc });
    return true;
}

asst::TilePack::result_type asst::TilePack::calc_(const Map::Level& level, double shift_x, double shift_y)
{
    LogTraceFunction;

    std::vector<std::vector<cv::Point2d>> pos;
    std::vector<std::vector<Map::Tile>> tiles;

    result_type result;
    const int w = level.get_width();
    const int h = level.get_height();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const auto tile = level.get_item(y, x);
            const cv::Point screen_pos =
                Map::TileCalc2::get_tile_screen_pos(level, y, x, false, { -shift_x, shift_y, 0 });
            const cv::Point screen_pos_side =
                Map::TileCalc2::get_tile_screen_pos(level, y, x, true, { -shift_x, shift_y, 0 });
            const asst::Point loc(x, y);

            const bool ret = proc_data(result.normal_tile_info, loc, screen_pos, tile);
            const bool ret_side = proc_data(result.side_tile_info, loc, screen_pos_side, tile);

            if (tile.tileKey == "tile_start" || tile.tileKey == "tile_end") {
                // 检查 screen_pos 是否在允许的范围内（带5%容差）
                constexpr double MIN_X = 0.0;
                constexpr double MAX_X = WindowWidthDefault;
                constexpr double MIN_Y = 0.0;
                constexpr double MAX_Y = WindowHeightDefault;
                constexpr double TOLERANCE = 0.05;

                constexpr double x_tolerance = MAX_X * TOLERANCE;
                constexpr double y_tolerance = MAX_Y * TOLERANCE;

                constexpr double min_x_bound = MIN_X - x_tolerance;
                constexpr double max_x_bound = MAX_X + x_tolerance;
                constexpr double min_y_bound = MIN_Y - y_tolerance;
                constexpr double max_y_bound = MAX_Y + y_tolerance;

                if (screen_pos.x < min_x_bound || screen_pos.x > max_x_bound || screen_pos.y < min_y_bound ||
                    screen_pos.y > max_y_bound) {
                    LogInfo << "Tile" << tile.tileKey << "at" << loc << ", screen position:" << screen_pos
                            << ", map has multi stages";
                    result.has_multi_stages = true;
                }
            }
            if (!ret || !ret_side) {
                Log.info("Tiles calc error!");
                return {};
            }
        }
    }
    auto retreat = Map::TileCalc2::get_retreat_screen_pos(level, result.has_multi_stages);
    auto skill = Map::TileCalc2::get_skill_screen_pos(level, result.has_multi_stages);
    result.retreat_button = { retreat.x, retreat.y };
    result.skill_button = { skill.x, skill.y };

    return result;
}
