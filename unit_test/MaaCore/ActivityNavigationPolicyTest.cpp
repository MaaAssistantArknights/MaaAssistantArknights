#include <catch2/catch_test_macros.hpp>

#include "Config/Miscellaneous/ActivityNavigationPolicy.h"

using namespace asst::activity_navigation;

TEST_CASE("Activity navigation timestamps have an explicit validated timezone")
{
    REQUIRE(parse_time("2026-10-17T08:00:00+08:00") == parse_time("2026-10-17T00:00:00Z"));
    REQUIRE(parse_time("2026-10-16T20:00:00-04:00") == parse_time("2026-10-17T00:00:00Z"));
    for (auto invalid : { "2026-10-17T00:00:00",
                          "2026-02-30T00:00:00Z",
                          "2026-10-17T24:00:00Z",
                          "2026-10-17T00:00:00+14:01",
                          "2026-10-17T00:00:00+aa:00",
                          "",
                          "bad" }) {
        REQUIRE_FALSE(parse_time(invalid));
    }
}

TEST_CASE("Known activity routes fail closed at time and mode boundaries")
{
    Route route;
    route.enabled = true;
    route.code = "YW-EX-1";
    route.clients = { "Official", "Bilibili" };
    route.normal = true;
    route.raid = true;
    route.opens_at = *parse_time("2026-10-16T16:00:00+08:00");
    route.closes_at = *parse_time("2026-10-23T03:59:00+08:00");
    REQUIRE(check(route, route.code, false, "Official", route.opens_at) == Status::Ready);
    REQUIRE(check(route, route.code, true, "Bilibili", route.opens_at) == Status::Ready);
    REQUIRE(check(route, route.code, true, "Official", route.opens_at - std::chrono::seconds(1)) == Status::Closed);
    REQUIRE(check(route, route.code, true, "Official", route.closes_at) == Status::Closed);
    REQUIRE(check(route, "YW-1", true, "Official", route.opens_at) == Status::WrongCode);
    REQUIRE(check(route, route.code, true, "YoStarEN", route.opens_at) == Status::WrongClient);
    REQUIRE(check(route, route.code, true, "", route.opens_at) == Status::WrongClient);
    route.normal = false;
    REQUIRE(check(route, route.code, true, "Official", route.opens_at) == Status::Ready);
    REQUIRE(reason(check(route, route.code, false, "Official", route.opens_at)) == "UnsupportedMode");
    REQUIRE(reason(check(route, route.code, true, "YoStarEN", route.opens_at)) == "WrongClient");
    REQUIRE(reason(check(route, route.code, true, "Official", route.closes_at)) == "Closed");
    route.normal = true;
    route.raid = false;
    REQUIRE(check(route, route.code, true, "Official", route.opens_at) == Status::UnsupportedMode);
    route.enabled = false;
    REQUIRE(check(route, route.code, false, "Official", route.opens_at) == Status::Disabled);
}

TEST_CASE("Existing OCR task routes do not synthesize user override policy")
{
    Route route;
    route.from_tasks = true;
    route.code = "YW-EX-1";
    const auto now = *parse_time("2026-10-17T00:00:00Z");
    REQUIRE(check(route, route.code, false, "Official", now) == Status::Ready);
    REQUIRE(check(route, route.code, true, "YoStarJP", now) == Status::Ready);
    REQUIRE(check(route, "YW-1", true, "Official", now) == Status::WrongCode);
}
