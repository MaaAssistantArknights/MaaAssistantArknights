#include "AsstCaller.h"

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <iostream>
#include <mutex>
#include <stdio.h>
#include <string>
#include <thread>

#ifdef SMOKE_TESTING
extern "C" void ASSTAPI AsstReleaseLifecycleTest();

namespace
{
constexpr AsstMsgId TaskChainStart = 10'001;
constexpr AsstMsgId SubTaskExtraInfo = 20'003;

struct LifecycleTestState
{
    std::mutex mutex;
    std::condition_variable changed;
    int start_count = 0;
    bool stop_observed = false;
};

void ASST_CALL lifecycle_test_callback(AsstMsgId msg, const char*, void* custom_arg)
{
    if (custom_arg == nullptr) {
        return;
    }

    auto& state = *static_cast<LifecycleTestState*>(custom_arg);
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (msg == TaskChainStart) {
            ++state.start_count;
        }
        else if (msg == SubTaskExtraInfo) {
            state.stop_observed = true;
        }
    }
    state.changed.notify_one();
}

bool run_lifecycle_cycle(AsstHandle handle, LifecycleTestState& state, int expected_start_count)
{
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.stop_observed = false;
    }
    if (AsstAppendTask(handle, "Debug", R"({"mode":"lifecycle"})") <= 0 ||
        AsstAppendTask(handle, "Debug", R"({"mode":"lifecycle"})") <= 0 || !AsstStart(handle)) {
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(state.mutex);
        if (!state.changed.wait_for(lock, std::chrono::seconds(5), [&state, expected_start_count]() {
                return state.start_count >= expected_start_count;
            })) {
            return false;
        }
    }

    auto stop_result = std::async(std::launch::async, [handle]() { return AsstStop(handle) != 0; });
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        if (!state.changed.wait_for(lock, std::chrono::seconds(5), [&state]() { return state.stop_observed; })) {
            std::cerr << "task did not observe stop" << std::endl;
            std::quick_exit(-1);
        }
    }

    const bool stop_returned_before_task_exit =
        stop_result.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    AsstReleaseLifecycleTest();
    if (stop_result.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
        std::cerr << "stop timed out" << std::endl;
        std::quick_exit(-1);
    }
    if (stop_returned_before_task_exit || !stop_result.get() || AsstRunning(handle)) {
        return false;
    }

    std::array<AsstTaskId, 1> tasks {};
    return AsstGetTasksList(handle, tasks.data(), tasks.size()) == 0;
}
}
#endif

int main([[maybe_unused]] int argc, char** argv)
{
    auto working_path = std::filesystem::path(argv[0]).parent_path();

    if (!std::filesystem::exists(working_path / "resource")) {
        std::cerr << "resource folder not found!" << std::endl;
        return -1;
    }

    // 可以将日志、调试图片等存到别的目录下,需要在最一开始调用。不调用默认保存到资源同目录
    // AsstSetUserDir(working_path.c_str());

    // 这里默认读取的是可执行文件同目录下 resource 文件夹里的资源
    if (!AsstLoadResource(working_path.string().c_str())) {
        std::cerr << "-------- load resource failed: official --------" << std::endl;
        return -1;
    }

#ifdef ASST_DEBUG
    if (argc > 1) {
        const std::string arg(argv[1]);

        if (arg == "Official") {
            std::cout << "Official type detected, using default resources." << std::endl;
        }
        else {
            std::cout << "load overseas_type: " << arg << std::endl;

            const auto overseas_path = working_path / "resource" / "global" / arg;
            if (!AsstLoadResource(overseas_path.string().c_str())) {
                std::cerr << "-------- load resource failed: " << arg << " --------" << std::endl;
                return -1;
            }
        }
    }
#endif

#ifdef SMOKE_TESTING
    LifecycleTestState lifecycle_state;
    auto ptr = AsstCreateEx(lifecycle_test_callback, &lifecycle_state);
#else
    auto ptr = AsstCreate();
#endif
    if (ptr == nullptr) {
        std::cerr << "create failed" << std::endl;
        return -1;
    }

#ifdef SMOKE_TESTING
    if (!run_lifecycle_cycle(ptr, lifecycle_state, 1) || !run_lifecycle_cycle(ptr, lifecycle_state, 2)) {
        std::cerr << "assistant lifecycle smoke test failed" << std::endl;
        AsstDestroy(ptr);
        return -1;
    }
    std::cout << "Assistant lifecycle smoke test passed." << std::endl;
    AsstDestroy(ptr);
    return 0;
#endif

#ifndef ASST_DEBUG
    AsstAsyncConnect(ptr, "adb", "127.0.0.1:5555", nullptr, true);
#else
    AsstAsyncConnect(ptr, "adb", "127.0.0.1:5555", "DEBUG", true);
#endif
    if (!AsstConnected(ptr)) {
        std::cerr << "connect failed" << std::endl;
        AsstDestroy(ptr);
        return -1;
    }

#ifdef ASST_DEBUG
    AsstAppendTask(ptr, "Debug", nullptr);
#else
    /* 详细参数可参考 docs / 集成文档.md */
    AsstAppendTask(ptr, "StartUp", nullptr);

    AsstAppendTask(ptr, "Fight", R"(
    {
        "stage": "1-7"
    }
    )");

    AsstAppendTask(ptr, "Recruit", R"(
    {
        "select":[4],
        "confirm":[3,4],
        "times":4
    }
    )");

    AsstAppendTask(ptr, "Infrast", R"(
    {
        "facility": ["Mfg", "Trade", "Power", "Control", "Reception", "Office", "Dorm"],
        "drones": "Money"
    }
    )");

    AsstAppendTask(ptr, "Mall", R"(
    {
        "shopping": true,
        "buy_first": [
            "许可"
        ],
        "black_list": [
            "家具",
            "碳"
        ]
    }
    )");

    AsstAppendTask(ptr, "Award", R"(
    {
        "award": true,
        "mail": true,
        "recruit": true,
        "orundum": true,
        "mining": true,
        "specialaccess": true
    }
    )");

    AsstAppendTask(ptr, "Roguelike", R"(
    {
        "theme": "Sarkaz",
        "mode": 1,
        "squad": "蓝图测绘分队",
        "roles": "稳扎稳打",
        "core_char": "维什戴尔"
    }
    )");
#endif

    AsstStart(ptr);

    while (AsstRunning(ptr)) {
        std::this_thread::yield();
    }

    AsstStop(ptr);
    AsstDestroy(ptr);

    return 0;
}
