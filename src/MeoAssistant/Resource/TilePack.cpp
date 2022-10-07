#include "TilePack.h"

#include "Utils/AsstConf.h"
ASST_SUPPRESS_CV_WARNINGS_START
#include <Arknights-Tile-Pos/TileCalc.hpp>
ASST_SUPPRESS_CV_WARNINGS_END

#include "Utils/Logger.hpp"

asst::TilePack::~TilePack() = default;

bool asst::TilePack::load(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) {
        return false;
    }

    try {
        m_tile_calculator = std::make_unique<Map::TileCalc>(WindowWidthDefault, WindowHeightDefault, path);
    }
    catch (const std::exception& e) {
        Log.error("Tile create failed", e.what());
        return false;
    }
    return m_tile_calculator != nullptr;
}

std::unordered_map<asst::Point, asst::TilePack::TileInfo> asst::TilePack::calc(const std::string& stage_code,
                                                                               bool side) const
{
    LogTraceFunction;

    std::vector<std::vector<cv::Point2d>> pos;
    std::vector<std::vector<Map::Tile>> tiles;

    bool ret = m_tile_calculator->run(stage_code, side, pos, tiles);

    Log.trace("After tiles calc run");
    if (!ret) {
        Log.info("Tiles calc error!");
        return {};
    }

    std::unordered_map<asst::Point, asst::TilePack::TileInfo> dst;

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
                Log.error("Unknown tile type:", tile.tileKey);
            }

            Point loc(static_cast<int>(x), static_cast<int>(y));
            dst.emplace(loc, TileInfo { static_cast<BuildableType>(tile.buildableType),
                                        static_cast<HeightType>(tile.heightType), key,
                                        Point(static_cast<int>(cv_p.x), static_cast<int>(cv_p.y)), loc });
        }
    }
    return dst;
}
