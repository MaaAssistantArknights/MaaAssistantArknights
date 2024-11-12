#pragma once

#include <meojson/json.hpp>
#include <numbers>
#include <opencv2/core/matx.hpp>
#include <opencv2/core/types.hpp>

#include "TileDef.hpp"

namespace Map::TileCalc2
{
using vec3d = cv::Vec3d;
using matrix4x4 = cv::Matx44d;
static constexpr double degree = std::numbers::pi / 180;

inline vec3d camera_pos(const Level& level, bool side = false, int width = 1280, int height = 720)
{
    const auto [x, y, z] = level.view[side ? 1 : 0];

    static constexpr double fromRatio = 9. / 16;
    static constexpr double toRatio = 3. / 4;
    const double ratio = static_cast<double>(height) / width;
    const double t = (fromRatio - ratio) / (fromRatio - toRatio);
    const vec3d pos_adj = { -1.4 * t, -2.8 * t, 0 };
    return { x + pos_adj[0], y + pos_adj[1], z + pos_adj[2] };
}

inline vec3d camera_euler_angles_yxz(const Level& /*level*/, bool side = false)
{
    if (side) {
        return { 10 * degree, 30 * degree, 0 };
    }

    return { 0, 30 * degree, 0 };
}

inline matrix4x4 camera_matrix_from_trans(
    const vec3d& pos,
    const vec3d& euler,
    double ratio,
    double fov_2_y = 20 * degree,
    double far_c = 1000,
    double near_c = 0.3)
{
    const double cos_y = std::cos(euler[0]);
    const double sin_y = std::sin(euler[0]);
    const double cos_x = std::cos(euler[1]);
    const double sin_x = std::sin(euler[1]);
    const double tan_f = std::tan(fov_2_y);

    const matrix4x4 translate = {
        1, 0, 0, -pos[0], //
        0, 1, 0, -pos[1], //
        0, 0, 1, -pos[2], //
        0, 0, 0, 1,
    };
    const matrix4x4 matrixY = {
        cos_y,  0, sin_y, 0, //
        0,      1, 0,     0, //
        -sin_y, 0, cos_y, 0, //
        0,      0, 0,     1,
    };
    const matrix4x4 matrixX = {
        1, 0,      0,      0, //
        0, cos_x,  -sin_x, 0, //
        0, -sin_x, -cos_x, 0, //
        0, 0,      0,      1,
    };
    const matrix4x4 proj = matrix4x4 {
        // clang-format off
        ratio / tan_f,  0,         0, 0,
        0,              1 / tan_f, 0, 0,
        0,              0,         -(far_c + near_c) / (far_c - near_c), -(far_c * near_c * 2) / (far_c - near_c),
        0,              0,         -1, 0,
        // clang-format on
    };

    return proj * matrixX * matrixY * translate;
}

inline cv::Point world_to_screen(const Level& level, const vec3d& world_pos, bool side, const vec3d& offset = {})
{
    static constexpr int width = 1280;
    static constexpr int height = 720;
    const vec3d pos_cam = camera_pos(level, side, width, height) + offset;
    const vec3d euler = camera_euler_angles_yxz(level, side);
    const matrix4x4 matrix = camera_matrix_from_trans(pos_cam, euler, static_cast<double>(height) / width);
    auto result = matrix * cv::Point3d(world_pos);
    result = result / result(3);
    result = (result + cv::Vec4d::ones()) / 2.;
    return {
        static_cast<int>(std::round(result(0) * width)),
        static_cast<int>(std::round((1 - result(1)) * height)),
    };
}

inline vec3d get_tile_world_pos(const Level& level, int tile_y, int tile_x)
{
    const int h = level.get_height();
    const int w = level.get_width();
    const auto& tile = level.get_item(tile_y, tile_x);
    return {
        tile_x - (w - 1) / 2.,
        (h - 1) / 2. - tile_y,
        tile.heightType * -0.4,
    };
}

inline auto get_tile_screen_pos(const Level& level, int tile_y, int tile_x, bool side = false, const vec3d& offset = {})
{
    return world_to_screen(level, get_tile_world_pos(level, tile_y, tile_x), side, offset);
}

static constexpr double rel_pos_x = 1.3143386840820312;
static constexpr double rel_pos_y = 1.314337134361267;
static constexpr double rel_pos_z = -0.3967874050140381;

inline auto get_retreat_screen_pos(const Level& level)
{
    const vec3d relative_pos = { -rel_pos_x, +rel_pos_y, rel_pos_z };
    return world_to_screen(level, relative_pos, true);
}

inline auto get_skill_screen_pos(const Level& level)
{
    const vec3d relative_pos = { +rel_pos_x, -rel_pos_y, rel_pos_z };
    return world_to_screen(level, relative_pos, true);
}

} // namespace Map::TileCalc2

