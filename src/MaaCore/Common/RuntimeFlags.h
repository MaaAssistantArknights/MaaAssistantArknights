#pragma once

#include <atomic>
#include <string_view>

namespace asst::flags
{
// 运行时开关，由 AsstSetStaticOption(StaticOptionKey::AutoSwitchTheme, "0"/"1") 设置。
// 默认 true：保持「识别失败时自动切换主题」的既有行为，不改变老用户的使用习惯。
inline std::atomic<bool> auto_switch_theme { true };

// 任务前置条件判定。未知条件一律视为放行，保证资源与核心可以分开升级。
inline bool enabled(std::string_view key) noexcept
{
    if (key == "auto_switch_theme") {
        return auto_switch_theme.load(std::memory_order_relaxed);
    }
    return true;
}
} // namespace asst::flags
