#include "TilePack.h"

#include "Common/AsstConf.h"
ASST_SUPPRESS_CV_WARNINGS_START
#include <Arknights-Tile-Pos/TileCalc.hpp>
ASST_SUPPRESS_CV_WARNINGS_END

#include "Utils/Logger.hpp"

bool asst::TilePack::load(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) {
        return false;
    }

    std::vector<json::value> tiles_array;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        const auto& file_path = entry.path();
        if (file_path.extension() != ".json") {
            continue;
        }
        auto json_opt = json::open(file_path);
        if (!json_opt) {
            Log.error("Failed to open json file:", file_path);
            return false;
        }
        auto& json = json_opt.value();
        if (json.is_array()) {
            // 兼容上游仓库的 levels.json
            // 有些用户习惯于在游戏更新了但maa还没发版前，自己手动更新下 levels.json，可以提前用
            tiles_array.insert(tiles_array.end(), std::make_move_iterator(json.as_array().begin()),
                               std::make_move_iterator(json.as_array().end()));
        }
        else if (json.is_object()) {
            tiles_array.emplace_back(std::move(*json_opt));
        }
        else {
            Log.error("Invalid json file:", file_path);
            return false;
        }
    }

    try {
        m_tile_calculator = std::make_shared<Map::TileCalc>(WindowWidthDefault, WindowHeightDefault, tiles_array);
    }
    catch (const std::exception& e) {
        Log.error("Tile create failed", e.what());
        return false;
    }
    return m_tile_calculator != nullptr;
}

bool asst::TilePack::contains(const std::string& any_key) const
{
    return m_tile_calculator->contains(any_key);
}

bool asst::TilePack::contains(const LevelKey& key) const
{
    return m_tile_calculator->contains(key);
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

std::unordered_map<asst::Point, asst::TilePack::TileInfo> asst::TilePack::calc(const std::string& any_key,
                                                                               bool side) const
{
    LogTraceFunction;

    std::vector<std::vector<cv::Point2d>> pos;
    std::vector<std::vector<Map::Tile>> tiles;

    bool ret = m_tile_calculator->run(any_key, side, pos, tiles);

    if (!ret) {
        Log.info("Tiles calc error!");
        return {};
    }

    return proc_data(pos, tiles);
}

std::unordered_map<asst::Point, asst::TilePack::TileInfo> asst::TilePack::calc(const LevelKey& key, bool side) const
{
    LogTraceFunction;

    std::vector<std::vector<cv::Point2d>> pos;
    std::vector<std::vector<Map::Tile>> tiles;

    bool ret = m_tile_calculator->run(key, side, pos, tiles);

    if (!ret) {
        Log.info("Tiles calc error!");
        return {};
    }

    return proc_data(pos, tiles);
}
