#pragma once

#include <chrono>
#include <cmath>
#include <functional>
#include <thread>

#include "Common/AsstTypes.h"

#ifdef _WIN32
// NOMINMAX 已由 AsstTypes.h 定义，SafeWindows.hpp 同样适用
#include "MaaUtils/SafeWindows.hpp"
#endif

namespace asst
{

// 高精度睡眠至指定时刻
// Windows 11 起最小化/被遮挡进程的 timeBeginPeriod 请求会被系统忽略，普通 sleep_until
// 的精度退回默认约 15.6ms 定时器粒度：首步睡过头后，后续所有绝对节拍点均已过期，
// sleep_until 立即返回导致 move 连发。高分辨率 waitable timer 不经过全局定时器分辨率
// 体系，窗口不可见时仍能兑现毫秒级节拍；创建失败（Win10 1803 之前）降级为普通睡眠。
// 非 Windows 平台的睡眠粒度足以兑现节拍，直接转发以保持调用点无平台分支
inline void high_res_sleep_until(std::chrono::steady_clock::time_point target)
{
#ifdef _WIN32
    const auto rel = target - std::chrono::steady_clock::now();
    if (rel <= std::chrono::steady_clock::duration::zero()) {
        return;
    }

    // 每次创建并销毁句柄：一次滑动约百步，syscall 开销可忽略，不引入缓存等复杂度
    HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (!timer) {
        std::this_thread::sleep_until(target);
        return;
    }

    LARGE_INTEGER due = {};
    // 负值表示相对当前时刻的超时，单位 100ns
    due.QuadPart = -std::chrono::duration_cast<std::chrono::nanoseconds>(rel).count() / 100;
    if (!SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE)) {
        // 未激活的 timer 永不置位，INFINITE 等待会永久卡死工作线程，失败即降级普通睡眠
        CloseHandle(timer);
        std::this_thread::sleep_until(target);
        return;
    }
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
#else
    std::this_thread::sleep_until(target);
#endif
}

// extra swipe 的额外位移方向
inline Point extra_swipe_offset(SwipeExtraDirection direction, int dist)
{
    switch (direction) {
    case SwipeExtraDirection::Up:
        return Point(0, -dist);
    case SwipeExtraDirection::Down:
        return Point(0, dist);
    case SwipeExtraDirection::Left:
        return Point(-dist, 0);
    case SwipeExtraDirection::Right:
        return Point(dist, 0);
    default:
        return Point(0, 0);
    }
}

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

// 带暂停检测的滑动插值执行器，滑动途中满足距离阈值时触发一次暂停回调
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
