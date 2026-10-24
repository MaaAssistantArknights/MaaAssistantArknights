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
#include <string_view>
#include <thread>

#ifdef SMOKE_TESTING
extern "C" void ASSTAPI AsstReleaseLifecycleTest();

namespace lifecycle_test
{
constexpr AsstMsgId AllTasksCompleted = 3;
constexpr AsstMsgId AsyncCallInfo = 4;
constexpr AsstMsgId TaskChainStart = 10'001;
constexpr AsstMsgId TaskChainCompleted = 10'002;
constexpr AsstMsgId TaskChainStopped = 10'004;
constexpr AsstMsgId SubTaskExtraInfo = 20'003;

struct LifecycleTestState
{
    std::mutex mutex;
    std::condition_variable changed;
    AsstHandle handle = nullptr;
    int start_count = 0;
    int ready_count = 0;
    int completed_count = 0;
    int stopped_count = 0;
    int all_completed_count = 0;
    int async_completed_count = 0;
    bool terminal_was_idle = true;
    bool restart_on_terminal = false;
    bool restart_succeeded = false;
};

void ASST_CALL lifecycle_test_callback(AsstMsgId msg, const char* details, void* custom_arg)
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
        else if (
            msg == SubTaskExtraInfo && std::string_view(details).find("LifecycleReady") != std::string_view::npos) {
            ++state.ready_count;
        }
        else if (msg == TaskChainCompleted) {
            ++state.completed_count;
        }
        else if (msg == AsyncCallInfo) {
            ++state.async_completed_count;
        }
        else if (msg == TaskChainStopped || msg == AllTasksCompleted) {
            state.terminal_was_idle &= !AsstRunning(state.handle);
            if (msg == TaskChainStopped) {
                ++state.stopped_count;
            }
            else {
                ++state.all_completed_count;
            }
            if (state.restart_on_terminal) {
                state.restart_on_terminal = false;
                // Match the GUI's completion -> Stop -> Append -> Start path.
                state.restart_succeeded = AsstStop(state.handle) && AsstAppendTask(state.handle, "Debug", "{}") > 0 &&
                                          AsstStart(state.handle);
            }
        }
    }
    state.changed.notify_one();
}

[[noreturn]] void fail(const char* message)
{
    std::cerr << "assistant lifecycle smoke test failed: " << message << std::endl;
    // Do not hang in AsstDestroy or a future destructor when testing a stuck worker.
    std::quick_exit(-1);
}

void require(bool condition, const char* message)
{
    if (!condition) {
        fail(message);
    }
}

template <typename Predicate>
void wait_for(LifecycleTestState& state, Predicate predicate, const char* message)
{
    std::unique_lock<std::mutex> lock(state.mutex);
    if (!state.changed.wait_for(lock, std::chrono::seconds(5), predicate)) {
        fail(message);
    }
}

void stop_promptly(AsstHandle handle)
{
    auto stop_result = std::async(std::launch::async, [handle]() { return AsstStop(handle) != 0; });
    require(stop_result.wait_for(std::chrono::seconds(5)) == std::future_status::ready, "Stop waited for the task");
    require(stop_result.get(), "Stop failed");
}

void require_empty(AsstHandle handle)
{
    std::array<AsstTaskId, 1> tasks {};
    require(AsstGetTasksList(handle, tasks.data(), tasks.size()) == 0, "pending tasks survived stop");
}

void run_lifecycle_tests(AsstHandle handle, LifecycleTestState& state)
{
    state.handle = handle;
    // An idle Stop must synchronously clear queued tasks without requiring a worker wakeup.
    require(AsstAppendTask(handle, "Debug", "{}") > 0, "idle append failed");
    stop_promptly(handle);
    require(!AsstRunning(handle), "idle Stop made the assistant busy");
    require_empty(handle);

    int expected_starts = 0;
    int expected_ready = 0;
    int expected_stopped = 0;
    int expected_all = 0;
    const auto start_blocked = [&](const char* params) {
        require(AsstAppendTask(handle, "Debug", params) > 0, "blocking append failed");
        require(AsstAppendTask(handle, "Debug", "{}") > 0, "pending append failed");
        require(AsstStart(handle), "Start failed");
        ++expected_starts;
        ++expected_ready;
        wait_for(state, [&]() { return state.ready_count == expected_ready; }, "task did not reach its gate");
    };

    for (int cycle = 0; cycle < 2; ++cycle) {
        start_blocked(R"({"mode":"lifecycle"})");
        stop_promptly(handle);
        stop_promptly(handle);
        require(AsstRunning(handle), "reported idle before the task returned");
        require(!AsstStart(handle), "restarted while the old task was blocked");
        require(AsstAppendTask(handle, "Debug", "{}") == 0, "accepted a task while stopping");
        AsstReleaseLifecycleTest();
        ++expected_stopped;
        wait_for(state, [&]() { return state.stopped_count == expected_stopped; }, "missing stop notification");
        require(!AsstRunning(handle), "stop notification preceded idle");
        require_empty(handle);
        std::lock_guard<std::mutex> lock(state.mutex);
        require(
            state.start_count == expected_starts && state.completed_count == 0 && state.all_completed_count == 0,
            "stopped run executed a pending task or reported normal completion");
    }

    start_blocked(R"({"mode":"lifecycle_internal_stop"})");
    AsstReleaseLifecycleTest();
    ++expected_stopped;
    wait_for(state, [&]() { return state.stopped_count == expected_stopped; }, "internal stop failed");
    require_empty(handle);

    // A terminal callback can immediately start a new run on the same instance.
    for (bool stop_run : { false, true }) {
        if (stop_run) {
            start_blocked(R"({"mode":"lifecycle"})");
            stop_promptly(handle);
            ++expected_stopped;
        }
        else {
            require(AsstAppendTask(handle, "Debug", R"({"mode":"lifecycle"})") > 0, "append failed");
            require(AsstStart(handle), "Start failed");
            ++expected_starts;
            ++expected_ready;
            wait_for(state, [&]() { return state.ready_count == expected_ready; }, "task did not reach its gate");
            ++expected_all;
        }
        {
            std::lock_guard<std::mutex> lock(state.mutex);
            state.restart_on_terminal = true;
            state.restart_succeeded = false;
        }
        AsstReleaseLifecycleTest();
        ++expected_starts;
        ++expected_all;
        wait_for(
            state,
            [&]() { return state.all_completed_count == expected_all; },
            "callback restart did not complete");
        std::lock_guard<std::mutex> lock(state.mutex);
        require(state.restart_succeeded && state.terminal_was_idle, "terminal callback could not restart");
        require(
            state.start_count == expected_starts && state.stopped_count == expected_stopped,
            "duplicate terminal notification or pending task was started");
    }

    // Exercise both orderings of an immediate Stop and a task returning naturally.
    for (int cycle = 0; cycle < 50; ++cycle) {
        int terminals;
        int ready;
        {
            std::lock_guard<std::mutex> lock(state.mutex);
            terminals = state.stopped_count + state.all_completed_count;
            ready = state.ready_count;
        }
        require(AsstAppendTask(handle, "Debug", R"({"mode":"lifecycle"})") > 0, "race append failed");
        require(AsstStart(handle), "race Start failed");
        wait_for(state, [&]() { return state.ready_count == ready + 1; }, "race task did not reach its gate");
        auto stop_result = std::async(std::launch::async, [handle]() { return AsstStop(handle) != 0; });
        AsstReleaseLifecycleTest();
        require(stop_result.wait_for(std::chrono::seconds(5)) == std::future_status::ready, "racing Stop hung");
        require(stop_result.get(), "racing Stop failed");
        wait_for(
            state,
            [&]() { return state.stopped_count + state.all_completed_count > terminals; },
            "racing Stop lost its terminal notification");
        require(!AsstRunning(handle), "racing Stop did not reach idle");
        require_empty(handle);
        std::lock_guard<std::mutex> lock(state.mutex);
        require(
            state.stopped_count + state.all_completed_count == terminals + 1 && state.terminal_was_idle,
            "racing Stop produced duplicate or premature terminal notifications");
    }

    // Stop immediately after Start, including before the worker has selected a task.
    for (int cycle = 0; cycle < 50; ++cycle) {
        int terminals;
        {
            std::lock_guard<std::mutex> lock(state.mutex);
            terminals = state.stopped_count + state.all_completed_count;
        }
        require(AsstAppendTask(handle, "Debug", "{}") > 0 && AsstStart(handle), "immediate Start failed");
        stop_promptly(handle);
        wait_for(
            state,
            [&]() { return state.stopped_count + state.all_completed_count > terminals; },
            "immediate Stop lost its terminal notification");
        require_empty(handle);
        std::lock_guard<std::mutex> lock(state.mutex);
        require(
            state.stopped_count + state.all_completed_count == terminals + 1 && state.terminal_was_idle,
            "immediate Stop produced duplicate or premature terminal notifications");
    }
    int all_completed;
    int starts;
    int completed;
    int ready;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        all_completed = state.all_completed_count;
        starts = state.start_count;
        completed = state.completed_count;
        ready = state.ready_count;
    }
    require(
        AsstAppendTask(handle, "Debug", "{}") > 0 && AsstAppendTask(handle, "Debug", "{}") > 0 && AsstStart(handle),
        "normal queue Start failed");
    wait_for(state, [&]() { return state.all_completed_count == all_completed + 1; }, "normal queue did not finish");
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        require(
            state.start_count == starts + 2 && state.completed_count == completed + 2 && state.terminal_was_idle,
            "normal queue lost a task or finished early");
    }

    require(AsstAsyncConnect(handle, "", "", "LIFECYCLE_TEST", false) > 0, "test connect was not queued");
    wait_for(state, [&]() { return state.ready_count == ready + 1; }, "test connect did not reach its gate");
    stop_promptly(handle);
    require(!AsstStart(handle), "Start overlapped an in-flight connection");
    AsstReleaseLifecycleTest();
    wait_for(state, [&]() { return state.async_completed_count == 1; }, "connect callback deadlocked during stop");
    require(!AsstRunning(handle), "test connect left a stale stop request");
    require(AsstAppendTask(handle, "Debug", "{}") > 0 && AsstStart(handle), "Start after test connect failed");
    wait_for(
        state,
        [&]() { return state.all_completed_count == all_completed + 2; },
        "post-connect run did not finish");
    std::cout << "Lifecycle checks: idle clear, nonblocking/repeated/internal stop, callback restart, "
                 "50 completion races, 50 immediate stops, normal queue and in-flight connect passed."
              << std::endl;
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
    lifecycle_test::LifecycleTestState lifecycle_state;
    auto ptr = AsstCreateEx(lifecycle_test::lifecycle_test_callback, &lifecycle_state);
#else
    auto ptr = AsstCreate();
#endif
    if (ptr == nullptr) {
        std::cerr << "create failed" << std::endl;
        return -1;
    }

#ifdef SMOKE_TESTING
    lifecycle_test::run_lifecycle_tests(ptr, lifecycle_state);
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
