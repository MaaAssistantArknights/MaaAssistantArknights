#pragma once

#include <cmath>
#include <functional>

#include "Common/AsstTypes.h"

namespace asst
{

// 三次样条插值函数，用于生成平滑的滑动曲线
// slope_0: 起点斜率，slope_1: 终点斜率，t: 插值进度 [0, 1]
inline double cubic_spline(double slope_0, double slope_1, double t)
{
    const double a = slope_0;
    const double b = -(2 * slope_0 + slope_1 - 3);
    const double c = -(-slope_0 - slope_1 + 2);
    return a * t + b * std::pow(t, 2) + c * std::pow(t, 3);
}

// 滑动插值执行器
// MoveFunc: bool(int x, int y) - 移动到指定位置的回调，返回 false 表示失败
// BoundsCheckFunc: bool(int x, int y) - 边界检查回调，返回 true 表示在边界内
template <typename MoveFunc, typename BoundsCheckFunc>
bool interpolate_swipe(
    int x1,
    int y1,
    int x2,
    int y2,
    int duration,
    int interval,
    double slope_in,
    double slope_out,
    MoveFunc&& move_func,
    BoundsCheckFunc&& bounds_check)
{
    for (int cur_time = interval; cur_time < duration; cur_time += interval) {
        double progress = cubic_spline(slope_in, slope_out, static_cast<double>(cur_time) / duration);
        int cur_x = static_cast<int>(std::lerp(static_cast<double>(x1), static_cast<double>(x2), progress));
        int cur_y = static_cast<int>(std::lerp(static_cast<double>(y1), static_cast<double>(y2), progress));

        if (!bounds_check(cur_x, cur_y)) {
            continue;
        }

        if (!move_func(cur_x, cur_y)) {
            return false;
        }
    }

    // 确保到达终点（如果在边界内）
    if (bounds_check(x2, y2)) {
        if (!move_func(x2, y2)) {
            return false;
        }
    }

    return true;
}

// 简化版本：不需要边界检查
template <typename MoveFunc>
bool interpolate_swipe(
    int x1,
    int y1,
    int x2,
    int y2,
    int duration,
    int interval,
    double slope_in,
    double slope_out,
    MoveFunc&& move_func)
{
    return interpolate_swipe(
        x1,
        y1,
        x2,
        y2,
        duration,
        interval,
        slope_in,
        slope_out,
        std::forward<MoveFunc>(move_func),
        [](int, int) { return true; });
}

// 带暂停检测的滑动插值执行器（用于 MinitouchController 的 swipe_with_pause 功能）
// PauseCheckFunc: bool(int cur_x, int cur_y, int start_x, int start_y) - 检查是否需要暂停
// PauseFunc: void() - 执行暂停操作
template <typename MoveFunc, typename BoundsCheckFunc, typename PauseCheckFunc, typename PauseFunc>
bool interpolate_swipe_with_pause(
    int x1,
    int y1,
    int x2,
    int y2,
    int duration,
    int interval,
    double slope_in,
    double slope_out,
    MoveFunc&& move_func,
    BoundsCheckFunc&& bounds_check,
    PauseCheckFunc&& pause_check,
    PauseFunc&& pause_func)
{
    bool pause_triggered = false;

    for (int cur_time = interval; cur_time < duration; cur_time += interval) {
        double progress = cubic_spline(slope_in, slope_out, static_cast<double>(cur_time) / duration);
        int cur_x = static_cast<int>(std::lerp(static_cast<double>(x1), static_cast<double>(x2), progress));
        int cur_y = static_cast<int>(std::lerp(static_cast<double>(y1), static_cast<double>(y2), progress));

        // 检查是否需要触发暂停
        if (!pause_triggered && pause_check(cur_x, cur_y, x1, y1)) {
            pause_triggered = true;
            pause_func();
        }

        if (!bounds_check(cur_x, cur_y)) {
            continue;
        }

        if (!move_func(cur_x, cur_y)) {
            return false;
        }
    }

    // 确保到达终点（如果在边界内）
    if (bounds_check(x2, y2)) {
        if (!move_func(x2, y2)) {
            return false;
        }
    }

    return true;
}

} // namespace asst
