#pragma once

// 截图前是否需要挪开真实光标、以及截图后是否该把光标还原的纯判定逻辑。
// 刻意不依赖 OpenCV / Windows，便于 unit_test 直接编译覆盖
// （见 unit_test/MaaCore/CaptureInterestTest.cpp）。

namespace asst::capture_interest
{
// 兴趣区外扩边距（画面坐标）。游戏内「鼠标指针比例」可调而 MAA 读不到，
// 64 是 1280x720 下约 5% 宽度的经验值：宁可多挪一次，也不要让自绘光标压住识别区。
inline constexpr int DefaultInterestMargin = 64;

// 判定「光标仍停在停靠点」的容差（px），用于吸收 DPI 与坐标取整误差。
inline constexpr int DefaultParkTolerance = 2;

// 光标相对截图画面的位置三态。
enum class CursorLocateState
{
    Inside,  // 已成功换算到画面坐标
    Outside, // 已成功换算，但落在客户区外
    Unknown, // 取坐标或换算失败，无法判定
};

constexpr const char* to_string(CursorLocateState state)
{
    switch (state) {
    case CursorLocateState::Inside:
        return "inside";
    case CursorLocateState::Outside:
        return "outside";
    default:
        return "unknown";
    }
}

// 是否需要挪开真实光标。
// - Outside：客户端不会把指针画进画面，故不挪。#18229 现场数据支持这一点：光标停在游戏窗口外的
//   166 帧里，暂停按钮亮度计数平均 599、仅 1 帧低于阈值，优于停靠成功的帧（平均 572、25/501 低于阈值）。
//   这也是最常见的挂机用法（鼠标停在 MAA 窗口上）。
// - Inside：只有确实落在本次识别要读的区域内才挪。
// - Unknown：拿不准就挪，与引入门控前的行为一致。
constexpr bool should_relocate(CursorLocateState state, bool hit_interest)
{
    switch (state) {
    case CursorLocateState::Outside:
        return false;
    case CursorLocateState::Inside:
        return hit_interest;
    default:
        return true;
    }
}

constexpr bool same_point(int lhs_x, int lhs_y, int rhs_x, int rhs_y, int tolerance = DefaultParkTolerance)
{
    const auto dx = static_cast<long long>(lhs_x) - rhs_x;
    const auto dy = static_cast<long long>(lhs_y) - rhs_y;
    const auto limit = static_cast<long long>(tolerance);
    return (dx < 0 ? -dx : dx) <= limit && (dy < 0 ? -dy : dy) <= limit;
}

// 光标是否落在兴趣区内（外扩 margin）。
// roi 宽或高为 0 表示「全图识别」（与 TaskInfo::roi 的约定一致），此时恒为 true，
// 即拿不准就按「必须挪开光标」处理 —— 与引入门控前的行为一致。
constexpr bool rect_hit(int cursor_x, int cursor_y, int roi_x, int roi_y, int roi_width, int roi_height, int margin)
{
    if (roi_width <= 0 || roi_height <= 0) {
        return true;
    }
    return cursor_x >= roi_x - margin && cursor_x < roi_x + roi_width + margin && cursor_y >= roi_y - margin &&
           cursor_y < roi_y + roi_height + margin;
}

// 截图后是否该把光标还原：只有它仍停在我们挪去的停靠点（没人动过）才还原。
// 用户自己动过就尊重其当前位置 —— 这取代了原先靠 BlockInput 冻结指针、
// 以保证「还原写不被硬件移动竞争覆盖」的做法。
constexpr bool should_restore_cursor(int now_x, int now_y, int parked_x, int parked_y, int tolerance = DefaultParkTolerance)
{
    return same_point(now_x, now_y, parked_x, parked_y, tolerance);
}
} // namespace asst::capture_interest
