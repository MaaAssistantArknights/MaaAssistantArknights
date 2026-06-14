#include <catch2/catch_all.hpp>

#include <string>
#include <vector>
#include <windows.h>

#include "UpdaterPlan.h"

// ============================================================================
// LoadPendingUpdatePlan integration test using testdata file
// ============================================================================

TEST_CASE("LoadPendingUpdatePlan loads real plan file", "[plan][integration]")
{
    std::wstring planFile = L"../testdata/文件名输出测试.json";

    PendingUpdatePlan plan;
    std::wstring failureReason;

    bool ok = LoadPendingUpdatePlan(planFile, plan, failureReason);
    CHECK(ok);
    if (!ok) {
        int len = WideCharToMultiByte(CP_UTF8, 0, failureReason.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string reason(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, failureReason.c_str(), -1, reason.data(), len, nullptr, nullptr);
        INFO("failureReason: " << reason);
    }
    REQUIRE(ok);

    CHECK(plan.packageType == L"ota");
    CHECK(plan.removeList.size() == 3);
    CHECK(plan.moveList.size() == 4);
}

TEST_CASE("LoadPendingUpdatePlan fails on missing file", "[plan][integration]")
{
    PendingUpdatePlan plan;
    std::wstring failureReason;

    bool ok = LoadPendingUpdatePlan(L"nonexistent_file.json", plan, failureReason);
    REQUIRE_FALSE(ok);
    REQUIRE_FALSE(failureReason.empty());
}
