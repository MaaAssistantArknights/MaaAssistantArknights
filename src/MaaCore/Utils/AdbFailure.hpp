#pragma once

#include <cstddef>
#include <string_view>

namespace asst::adb
{
/// 失败输出的可判长度上限；超过多半是截图数据只传了一半，不再细判
inline constexpr size_t MaxFailureOutputSize = 4096;

/**
 * @brief 判定一次 adb 命令的失败输出属于哪一类
 *
 * adb 进程返回非零有两类截然不同的含义：
 * - 设备层面的失败，adb 自己报的（`device offline` / `device 'xxx' not found` /
 *   `no devices/emulators found` / `failed to connect` 等），连接确实断了，重连才有意义；
 * - 命令自身的失败，命令已经送到设备、执行完返回非零，如 am start 找不到目标组件时的
 *   `Error type 3` + `Error: Activity class {...} does not exist.`，重连多少次结果都一样。
 *
 * 判错方向要一致：认不出来时一律按设备不可达处理，宁可多走一次重连阶梯，也不能把一次
 * 命令级失败当成掉线，把好好的连接拆了，只为最后报一句 ｢重连失败｣ 而已。
 *
 * @param output 命令失败时的输出（adb 的报错走 stderr，与 stdout 合流）
 * @return true 表示设备不可达，值得重连
 */
[[nodiscard]] inline bool is_device_unreachable(std::string_view output)
{
    if (output.size() > MaxFailureOutputSize) {
        return true;
    }

    // 没有输出可判断：无法证明命令真的送到了设备，按设备不可达处理
    if (output.find_first_not_of(" \t\r\n") == std::string_view::npos) {
        return true;
    }

    static constexpr std::string_view DeviceLostMarkers[] = {
        "not found",       "offline",           "unauthorized",
        "device still",    "no devices",        "no device/emulator",
        "cannot connect",  "failed to connect", "protocol fault",
        "transport error", "connection reset",  "more than one device",
        "closed",
    };
    for (auto marker : DeviceLostMarkers) {
        if (output.find(marker) != std::string_view::npos) {
            return true;
        }
    }

    static constexpr std::string_view CommandFailedMarkers[] = {
        "Error type",        "Error:",          "error:", "Exception", "java.lang.", "Permission Denial",
        "SecurityException", "Unknown command", "usage:", "Warning:",
    };
    for (auto marker : CommandFailedMarkers) {
        if (output.find(marker) != std::string_view::npos) {
            return false;
        }
    }

    return true;
}

/**
 * @brief 判定 am start 的失败输出是否属于 ｢设备上没有目标客户端｣
 *
 * am 找不到目标组件时的输出（Android 侧）：
 * @code
 * Starting: Intent { cmp=com.hypergryph.arknights/com.u8.sdk.U8UnityContext }
 * Error type 3
 * Error: Activity class {com.hypergryph.arknights/com.u8.sdk.U8UnityContext} does not exist.
 * @endcode
 * 说明这台设备上没有 MAA 预期的客户端：客户端没装、连错了模拟器实例，或客户端换了 Activity 名。
 *
 * @param output am start 的失败输出
 * @return true 表示设备上没有 MAA 预期的客户端
 */
[[nodiscard]] inline bool is_client_missing(std::string_view output)
{
    return output.find("Error type") != std::string_view::npos ||
           output.find("does not exist") != std::string_view::npos;
}
} // namespace asst::adb
