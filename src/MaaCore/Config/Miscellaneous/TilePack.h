#pragma once

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"
#include "Config/AbstractConfig.h"

#include <Arknights-Tile-Pos/TileDef.hpp>

namespace asst
{
    class TilePack final : public SingletonHolder<TilePack>, public AbstractConfig
    {
    public:
        using LevelKey = Map::LevelKey;
        using LazyMap = std::vector<std::pair<LevelKey, std::filesystem::path>>;

    public:
        enum class HeightType
        {
            Invalid = -1,
            Highland = 0,
            Floor = 1
        };
        // clang-format off
        enum class TileKey
        {
            Invalid = -1,
            Forbidden, // 不能放干员，敌人也不能走
            Wall,      // 可以放高台干员的位置
            Road,      // 可以放地面干员，敌人也可以走
            Home,      // 蓝门（可能还有其他的情况）
            EnemyHome, // 红门（可能还有其他的情况）
            Airport,   // 无人机始发站
            Floor,     // 不能放干员，但敌人可以走
            Hole,      // 空降兵掉下去的地方（
            Telin,     // 传送门入口
            Telout,    // 传送门出口
            Grass,     // 草地，带有隐匿，可以放干员，部分地块敌人可以走
            DeepSea,   // 水，不能直接放干员（要借助泳圈道具），敌人可以走，但敌人会持续掉血
            Volcano,   // 岩浆地块，可以放干员，敌人也可以走，但是会持续掉血
            Healing,   // 治疗地块，可以放干员，敌人也可以走，会给干员回血
            Fence,     // 敌人不会走，但可以放干员的地面位置
        }; // clang-format on

        struct TileInfo
        {
            battle::LocationType buildable = battle::LocationType::Invalid;
            HeightType height = HeightType::Invalid;
            TileKey key = TileKey::Invalid;
            Point pos; // 像素坐标
            Point loc; // 格子位置
        };

    public:
        virtual ~TilePack() override = default;

        template <typename KeyT>
        std::optional<LazyMap::value_type> find(const KeyT& key) const
        {
            for (const auto& pair : m_summarize) {
                if (pair.first == key) {
                    return pair;
                }
            }
            return std::nullopt;
        }

        template <typename KeyT>
        std::unordered_map<Point, TileInfo> calc(const KeyT& key, bool side, double shift_x = 0,
                                                 double shift_y = 0) const
        {
            auto file_opt = find(key);
            if (!file_opt) {
                return {};
            }
            return calc_(file_opt->second, side, shift_x, shift_y);
        }

    protected:
        virtual bool parse(const json::value& json) override;

    private:
        std::unordered_map<Point, TileInfo> calc_(const std::filesystem::path& filepath, bool side, double shift_x,
                                                  double shift_y) const;
        LazyMap m_summarize;
    };

    inline static auto& Tile = TilePack::get_instance();
} // namespace asst
