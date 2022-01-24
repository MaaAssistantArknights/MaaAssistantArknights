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

    for (size_t y = 0; y < pos.size(); ++y) {
        for (size_t x = 0; x < pos[y].size(); ++x) {
            const auto& cv_p = pos[y][x];
            const auto& tile = tiles[y][x];

            dst.emplace_back(
                TileInfo{
                    static_cast<BuildableType>(tile.buildableType),
                    static_cast<HeightType>(tile.heightType),
                    Point(static_cast<int>(cv_p.x), static_cast<int>(cv_p.y))
                });
        }
    }
    return dst;
}
