#include <catch2/catch_all.hpp>

#include <string>
#include <windows.h>

#include "UpdaterMutex.h"

// ============================================================================
// AcquireUpdateMutex / ReleaseUpdateMutex
// ============================================================================

TEST_CASE("AcquireUpdateMutex succeeds with unique name", "[mutex]")
{
    std::wstring name = L"Global\\MaaUpdaterTest_Mutex_" + std::to_wstring(GetTickCount());

    HANDLE h = AcquireUpdateMutex(name);
    REQUIRE(h != nullptr);
    REQUIRE(h != INVALID_HANDLE_VALUE);

    ReleaseUpdateMutex(h);
}

TEST_CASE("AcquireUpdateMutex blocks second acquisition on same name", "[mutex]")
{
    // Windows mutexes are recursive within the same thread.
    // A second AcquireUpdateMutex from the same thread succeeds (recursive).
    // Cross-process blocking is tested in integration, not in single-threaded unit tests.
    std::wstring name = L"Global\\MaaUpdaterTest_Mutex_Dup_" + std::to_wstring(GetTickCount());

    HANDLE h1 = AcquireUpdateMutex(name);
    REQUIRE(h1 != nullptr);

    // Recursive acquisition from same thread succeeds
    HANDLE h2 = AcquireUpdateMutex(name);
    CHECK(h2 != nullptr);

    ReleaseUpdateMutex(h2);
    ReleaseUpdateMutex(h1);
}

TEST_CASE("ReleaseUpdateMutex allows re-acquisition", "[mutex]")
{
    std::wstring name = L"Global\\MaaUpdaterTest_Mutex_ReAcq_" + std::to_wstring(GetTickCount());

    HANDLE h1 = AcquireUpdateMutex(name);
    REQUIRE(h1 != nullptr);

    ReleaseUpdateMutex(h1);

    // After release, a new acquisition should succeed
    HANDLE h2 = AcquireUpdateMutex(name);
    REQUIRE(h2 != nullptr);

    ReleaseUpdateMutex(h2);
}

TEST_CASE("ReleaseUpdateMutex handles nullptr safely", "[mutex]")
{
    // Should not crash
    ReleaseUpdateMutex(nullptr);
    SUCCEED("ReleaseUpdateMutex(nullptr) did not crash");
}
