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

bool asst::TilePack::load(const std::filesystem::path& path)
{
    LogTraceFunction;
    Log.info("load", path);

    if (!std::filesystem::exists(path)) {
        return false;
    }

    std::list<json::value> tiles_array;
    std::queue<std::pair<std::filesystem::path, std::string>> file_strings;

    std::atomic_bool eoq = false;
    std::mutex queue_mut;
    std::condition_variable condvar;

    using result_type = std::optional<std::list<json::value>>;
    std::vector<std::future<result_type>> workers;

    {
        auto n_workers = std::max(1U, std::thread::hardware_concurrency());
        workers.reserve(n_workers);
        for (auto thi = 0U; thi < n_workers; ++thi) {
            workers.emplace_back(std::async(std::launch::async, [&]() -> result_type {
                std::list<json::value> result;
                while (true) {
                    std::unique_lock lk { queue_mut };
                    condvar.wait(lk, [&]() -> bool { return !file_strings.empty() || eoq.load(); });
                    if (file_strings.empty()) return result;

                    std::string buf {};
                    buf.swap(file_strings.front().second);
                    auto path = file_strings.front().first;
                    file_strings.pop();
                    lk.unlock();

                    auto json_opt = json::parse(buf);
                    if (!json_opt) {
                        Log.error("Unable to parse json file:", path);
                        eoq.store(true);
                        return std::nullopt;
                    }

                    auto& json = json_opt.value();
                    if (json.is_array()) {
                        // 兼容上游仓库的 levels.json
                        // 有些用户习惯于在游戏更新了但maa还没发版前，自己手动更新下 levels.json，可以提前用
                        result.insert(tiles_array.end(), std::make_move_iterator(json.as_array().begin()),
                                      std::make_move_iterator(json.as_array().end()));
                    }
                    else if (json.is_object()) {
                        result.emplace_back(std::move(json));
                    }
                    else {
                        Log.error("Invalid json file:", path);
                        eoq.store(true);
                        return std::nullopt;
                    }
                }
            }));
        }
    }

    for (const auto& entry : std::filesystem::directory_iterator(path)) {

        if (eoq.load()) break; // this means parsing went wrong

        const auto& file_path = entry.path();
        if (file_path.extension() != ".json") continue;

        const auto f_size = std::filesystem::file_size(file_path);
        std::ifstream ifs(file_path, std::ios::in);
        auto buf = std::string(f_size, '\0');
        ifs.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        {
            std::unique_lock lk(queue_mut);
            file_strings.push(std::make_pair(file_path, std::move(buf)));
        }
        condvar.notify_one();
    }

    eoq.store(true);
    condvar.notify_all();

    auto result = std::transform_reduce(
        workers.begin(), workers.end(), std::optional { std::list<json::value> {} },
        [](result_type lhs, result_type rhs) -> result_type {
            if (!lhs || !rhs) return std::nullopt;
            lhs->splice(lhs->end(), std::move(rhs).value());
            return lhs;
        },
        [&](std::future<result_type>& fut) -> result_type {
            while (fut.wait_for(std::chrono::milliseconds(10)) == std::future_status::timeout)
                condvar.notify_all(); // is this necessary?
            return fut.get();
        });

    if (!result) return false;
    Log.info("got", result->size(), "maps");

    try {
        // TODO: this move has no effect
        m_tile_calculator =
            std::make_shared<Map::TileCalc>(WindowWidthDefault, WindowHeightDefault, std::move(result).value());
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

std::unordered_map<asst::Point, asst::TilePack::TileInfo> asst::TilePack::calc(const std::string& any_key, bool side,
                                                                               double shift_x, double shift_y) const
{
    LogTraceFunction;

    std::vector<std::vector<cv::Point2d>> pos;
    std::vector<std::vector<Map::Tile>> tiles;

    bool ret = m_tile_calculator->run(any_key, side, pos, tiles, shift_x, shift_y);

    if (!ret) {
        Log.info("Tiles calc error!");
        return {};
    }

    return proc_data(pos, tiles);
}

std::unordered_map<asst::Point, asst::TilePack::TileInfo> asst::TilePack::calc(const LevelKey& key, bool side,
                                                                               double shift_x, double shift_y) const
{
    LogTraceFunction;

    std::vector<std::vector<cv::Point2d>> pos;
    std::vector<std::vector<Map::Tile>> tiles;

    bool ret = m_tile_calculator->run(key, side, pos, tiles, shift_x, shift_y);

    if (!ret) {
        Log.info("Tiles calc error!");
        return {};
    }

    return proc_data(pos, tiles);
}
