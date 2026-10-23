#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>

#include "Utils/AdbFailure.hpp"

namespace
{
// 用户现场：MuMu 实例上没有安装明日方舟时 am start 的真实输出（asst.log 2026-10-23 14:53:09）
constexpr std::string_view AmStartActivityMissing =
    "Starting: Intent { cmp=com.hypergryph.arknights/com.u8.sdk.U8UnityContext }\n"
    "Error type 3\n"
    "Error: Activity class {com.hypergryph.arknights/com.u8.sdk.U8UnityContext} does not exist.\n";

// am start 只报 ｢组件找不到｣ ，不报 Error type 时的形态（不同 Android 版本措辞不一）
constexpr std::string_view ActivityClassMissing =
    "Error: Activity class {com.hypergryph.arknights/com.u8.sdk.U8UnityContext} does not exist.";

// 客户端已在运行时 am start 的正常输出（退出码为 0，不该被判成失败，仅留作回归样本）
constexpr std::string_view ActivityNotStarted =
    "Warning: Activity not started, its current task has been brought to the front.";
} // namespace

TEST_CASE("asst::adb::is_device_unreachable", "[adb_failure]") {
    SECTION("adb 报的设备不可达，一律仍走重连阶梯") {
        REQUIRE(asst::adb::is_device_unreachable("adb: device offline"));
        REQUIRE(asst::adb::is_device_unreachable("adb.exe: device 'emulator-5554' not found"));
        REQUIRE(asst::adb::is_device_unreachable("adb: device unauthorized"));
        REQUIRE(asst::adb::is_device_unreachable("adb: device still connecting"));
        REQUIRE(asst::adb::is_device_unreachable("adb: no devices/emulators found"));
        REQUIRE(asst::adb::is_device_unreachable("adb: failed to connect to 127.0.0.1:16384"));
        REQUIRE(asst::adb::is_device_unreachable("adb: cannot connect to 127.0.0.1:16384: Connection refused"));
        REQUIRE(asst::adb::is_device_unreachable("adb: more than one device/emulator"));
        REQUIRE(asst::adb::is_device_unreachable("adb: error: closed"));
        REQUIRE(asst::adb::is_device_unreachable("protocol fault (couldn't read status)"));
        REQUIRE(asst::adb::is_device_unreachable("adb: transport error"));
    }

    SECTION("认不出来的情况按设备不可达处理，保持原有恢复能力") {
        REQUIRE(asst::adb::is_device_unreachable(""));
        REQUIRE(asst::adb::is_device_unreachable(" \r\n"));
        REQUIRE(asst::adb::is_device_unreachable("something odd happened"));
        REQUIRE(asst::adb::is_device_unreachable(std::string(asst::adb::MaxFailureOutputSize + 1, 'x')));
    }

    SECTION("命令执行失败不该重连，重连多少次都一样") {
        REQUIRE_FALSE(asst::adb::is_device_unreachable(AmStartActivityMissing));
        REQUIRE_FALSE(
            asst::adb::is_device_unreachable("Operation not allowed: java.lang.SecurityException: Permission Denial"));
        REQUIRE_FALSE(asst::adb::is_device_unreachable("Error: inject keyevent failed"));
        REQUIRE_FALSE(asst::adb::is_device_unreachable("Unknown command: frobnicate"));
        REQUIRE_FALSE(asst::adb::is_device_unreachable(ActivityNotStarted));
    }
}

TEST_CASE("asst::adb::is_client_missing", "[adb_failure]") {
    REQUIRE(asst::adb::is_client_missing(AmStartActivityMissing));
    REQUIRE(asst::adb::is_client_missing(ActivityClassMissing));
    REQUIRE(asst::adb::is_client_missing("Error type 3"));

    REQUIRE_FALSE(asst::adb::is_client_missing(""));
    REQUIRE_FALSE(asst::adb::is_client_missing("adb: device offline"));
    REQUIRE_FALSE(asst::adb::is_client_missing(ActivityNotStarted));
}