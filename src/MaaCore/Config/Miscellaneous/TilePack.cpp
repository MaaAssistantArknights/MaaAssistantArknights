#include "TilePack.h"

#include "Common/AsstConf.h"
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

ASST_SUPPRESS_CV_WARNINGS_START
#include <Arknights-Tile-Pos/TileCalc.hpp>
ASST_SUPPRESS_CV_WARNINGS_END

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
        if (!std::filesystem::exists(filepath)) {
            Log.error("file not exists", filepath);
            return false;
        }
        m_summarize.emplace_back(std::move(level_key), std::move(filepath));
    }
    return true;
}

std::unordered_map<asst::Point, asst::TilePack::TileInfo> proc_data(const std::vector<std::vector<cv::Point2d>>& pos,
                                                                    const std::vector<std::vector<Map::Tile>>& tiles)
{
    using namespace asst;
    using TileKey = asst::TilePack::TileKey;
    using TileInfo = asst::TilePack::TileInfo;

    LogTraceFunction;

    std::unordered_map<asst::Point, TileInfo> dst;

    static const std::unordered_map<std::string, TileKey> TileKeyMapping = {
        { "tile_forbidden", TileKey::Forbidden }, { "tile_wall", TileKey::Wall },
        { "tile_road", TileKey::Road },           { "tile_end", TileKey::Home },
        { "tile_start", TileKey::EnemyHome },     { "tile_flystart", TileKey::Airport },
        { "tile_floor", TileKey::Floor },         { "tile_hole", TileKey::Hole },
        { "tile_telin", TileKey::Telin },         { "tile_telout", TileKey::Telout },
        { "tile_grass", TileKey::Grass },         { "tile_deepsea", TileKey::DeepSea },
        { "tile_deepwater", TileKey::DeepSea },   { "tile_volcano", TileKey::Volcano },
        { "tile_healing", TileKey::Healing },     { "tile_fence", TileKey::Fence },
    };

    for (size_t y = 0; y < pos.size(); ++y) {
        for (size_t x = 0; x < pos[y].size(); ++x) {
            const auto& cv_p = pos[y][x];
            const auto& tile = tiles[y][x];

            auto key = TileKey::Invalid;
            if (auto iter = TileKeyMapping.find(tile.tileKey); iter != TileKeyMapping.cend()) {
                key = iter->second;
            }
            else {
                key = TileKey::Invalid;
                Log.warn("Unknown tile type:", tile.tileKey);
            }

            Point loc(static_cast<int>(x), static_cast<int>(y));
            dst.emplace(loc, TileInfo { static_cast<battle::LocationType>(tile.buildableType),
                                        static_cast<TilePack::HeightType>(tile.heightType), key,
                                        Point(static_cast<int>(cv_p.x), static_cast<int>(cv_p.y)), loc });
        }
    }
    return dst;
}

std::unordered_map<asst::Point, asst::TilePack::TileInfo> asst::TilePack::calc_(const std::filesystem::path& filepath,
                                                                                bool side, double shift_x,
                                                                                double shift_y) const
{
    LogTraceFunction;

    auto json_opt = json::open(filepath);
    if (!json_opt) {
        Log.info("failed to open", filepath);
        return {};
    }

    std::vector<std::vector<cv::Point2d>> pos;
    std::vector<std::vector<Map::Tile>> tiles;

    Map::Level level(*json_opt);
    Map::TileCalc calcer(WindowWidthDefault, WindowHeightDefault);

    bool ret = calcer.run(level, side, pos, tiles, shift_x, shift_y);

    if (!ret) {
        Log.info("Tiles calc error!");
        return {};
    }

    return proc_data(pos, tiles);
}
