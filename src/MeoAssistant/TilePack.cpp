#include "TilePack.h"

#include <Arknights-Tile-Pos/TileCalc.hpp>

#include "Logger.hpp"

asst::TilePack::~TilePack() = default;

bool asst::TilePack::load(const std::string & dir)
{
    LogTraceFunction;

    constexpr static const char* filename = "/levels.json";

    try {
        m_tile_calculator = std::make_unique<Map::TileCalc>(
            WindowWidthDefault, WindowHeightDefault, dir + filename);
    }
    catch (std::exception& e) {
        m_last_error = e.what();
        return false;
    }
    return m_tile_calculator != nullptr;
}

std::vector<asst::TilePack::TileInfo> asst::TilePack::calc(const std::string & stage_code, bool side) const
{
    LogTraceFunction;

    std::vector<std::vector<cv::Point2d>> pos;
    std::vector<std::vector<Map::Tile>> tiles;

    bool ret = m_tile_calculator->run(stage_code, side, pos, tiles);

    Log.trace("After tiles cacl run");
    if (!ret) {
        Log.info("Tiles calc error!");
        return std::vector<TileInfo>();
    }

    std::vector<TileInfo> dst;

    static const std::unordered_map<std::string, TileKey> TileKeyMapping = {
        { "tile_forbidden", TileKey::Forbidden },
        { "tile_wall", TileKey::Wall },
        { "tile_road", TileKey::Road },
        { "tile_end", TileKey::Home },
        { "tile_start", TileKey::EnemyHome }
    };

    for (size_t y = 0; y < pos.size(); ++y) {
        for (size_t x = 0; x < pos[y].size(); ++x) {
            const auto& cv_p = pos[y][x];
            const auto& tile = tiles[y][x];

            TileKey key = TileKey::Invaild;
            if (auto iter = TileKeyMapping.find(tile.tileKey);
                iter != TileKeyMapping.cend()) {
                key = iter->second;
            }
            else {
                key = TileKey::Invaild;
                Log.error("Unknown tile type:", tile.tileKey);
            }

            dst.emplace_back(
                TileInfo{
                    static_cast<BuildableType>(tile.buildableType),
                    static_cast<HeightType>(tile.heightType),
                    key,
                    Point(static_cast<int>(cv_p.x), static_cast<int>(cv_p.y))
                });
        }
    }
    return dst;
}
