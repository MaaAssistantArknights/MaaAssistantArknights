#pragma once

#include <string>
#include <vector>

#include <meojson/json.hpp>
#include <opencv2/core.hpp>

namespace Map
{
struct LevelKey
{
    std::string stageId;
    std::string code;
    std::string levelId;
    std::string name;

    bool empty_or_equal(const std::string& lhs, const std::string& rhs) const noexcept
    {
        return (lhs.empty() || rhs.empty()) ? true : lhs == rhs;
    }

    bool operator==(const LevelKey& other) const noexcept
    {
        return empty_or_equal(stageId, other.stageId) && empty_or_equal(code, other.code)
               && empty_or_equal(levelId, other.levelId) && empty_or_equal(name, other.name);
    }

    bool operator==(const std::string& any_key) const noexcept
    {
        if (any_key.empty()) {
            return false;
        }
        return empty_or_equal(stageId, any_key) || empty_or_equal(code, any_key)
               || empty_or_equal(levelId, any_key) || empty_or_equal(name, any_key);
    }
};

struct Tile
{
    int heightType = 0;
    int buildableType = 0;
    std::string tileKey;
};

class Level
{
public:
    explicit Level(const json::value& data);
    Level() = default;

    int get_width() const { return width; }

    int get_height() const { return height; }

    Tile get_item(int y, int x) const { return tiles[y][x]; }

    std::vector<cv::Point3d> view {};
    LevelKey key {};

private:
    int height = 0;
    int width = 0;
    std::vector<std::vector<Tile>> tiles;
};

inline Level::Level(const json::value& data)
{
    key.stageId = data.at("stageId").as_string();
    key.code = data.at("code").as_string();
    key.levelId = data.at("levelId").as_string();
    key.name = data.get("name", "null");
    this->height = data.at("height").as_integer();
    this->width = data.at("width").as_integer();
    for (const json::value& point_data : data.at("view").as_array()) {
        cv::Point3d tmp;
        auto point_array = point_data.as_array();
        tmp.x = point_array[0].as_double();
        tmp.y = point_array[1].as_double();
        tmp.z = point_array[2].as_double();
        this->view.emplace_back(tmp);
    }
    for (const json::value& row : data.at("tiles").as_array()) {
        std::vector<Tile> tmp;
        tmp.reserve(this->width);
        for (const json::value& tile : row.as_array()) {
            tmp.emplace_back(Tile { tile.at("heightType").as_integer(),
                                    tile.at("buildableType").as_integer(),
                                    tile.get("tileKey", std::string()) });
        }
        tiles.emplace_back(std::move(tmp));
    }
}
} // namespace Map
