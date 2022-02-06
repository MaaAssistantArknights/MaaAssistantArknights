#pragma once

#include "AbstractResource.h"
#include "AsstDef.h"

#include <memory>

namespace Map
{
    class TileCalc;
}

namespace asst
{
    class TilePack final : public AbstractResource
    {
    public:
        enum class BuildableType
        {
            Invalid = -1,
            None = 0,
            Melee = 1,
            Ranged = 2,
            All = 3
        };
        enum class HeightType
        {
            Invalid = -1,
            Highland = 0,
            Floor = 1
        };
        enum class TileKey
        {
            Invalid = -1,
            Forbidden,
            Wall,
            Road,
            Home,
            EnemyHome,
            Floor,
            Hole,
            Telin,
            Telout
        };

        struct TileInfo
        {
            BuildableType buildable = BuildableType::Invalid;
            HeightType height = HeightType::Invalid;
            TileKey key = TileKey::Invalid;
            Point pos;  // 像素坐标
            Point loc;  // 格子位置
        };
    public:
        using AbstractResource::AbstractResource;
        virtual ~TilePack();

        virtual bool load(const std::string& dir) override;

        std::unordered_map<Point, TileInfo> calc(const std::string& stage_code, bool side) const;

    private:
        std::shared_ptr<Map::TileCalc> m_tile_calculator = nullptr;
    };
}
