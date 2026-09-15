#include <catch2/catch_test_macros.hpp>

#include <string>

#include "Controller/CaptureInterest.hpp"

using namespace asst::capture_interest;

TEST_CASE("rect_hit: point inside the roi")
{
    // roi = [50, 250) x [50, 250)
    REQUIRE(rect_hit(100, 100, 50, 50, 200, 200, 0));
    REQUIRE(rect_hit(50, 50, 50, 50, 200, 200, 0)); // 左上角含
    REQUIRE(rect_hit(249, 249, 50, 50, 200, 200, 0));
    REQUIRE_FALSE(rect_hit(250, 250, 50, 50, 200, 200, 0));
    REQUIRE_FALSE(rect_hit(49, 100, 50, 50, 200, 200, 0));
    REQUIRE_FALSE(rect_hit(100, 49, 50, 50, 200, 200, 0));
}

TEST_CASE("rect_hit: margin expands the roi")
{
    constexpr int Margin = DefaultInterestMargin;
    constexpr int RoiX = 50;
    constexpr int RoiY = 50;
    constexpr int RoiW = 200;
    constexpr int RoiH = 200;

    // 恰好落在 margin 边缘算命中
    REQUIRE(rect_hit(RoiX - Margin, RoiY - Margin, RoiX, RoiY, RoiW, RoiH, Margin));
    REQUIRE(rect_hit(RoiX + RoiW + Margin - 1, RoiY + RoiH + Margin - 1, RoiX, RoiY, RoiW, RoiH, Margin));
    // 再往外 1px 就不算
    REQUIRE_FALSE(rect_hit(RoiX - Margin - 1, RoiY, RoiX, RoiY, RoiW, RoiH, Margin));
    REQUIRE_FALSE(rect_hit(RoiX + RoiW + Margin, RoiY, RoiX, RoiY, RoiW, RoiH, Margin));
    REQUIRE_FALSE(rect_hit(RoiX, RoiY - Margin - 1, RoiX, RoiY, RoiW, RoiH, Margin));
    REQUIRE_FALSE(rect_hit(RoiX, RoiY + RoiH + Margin, RoiX, RoiY, RoiW, RoiH, Margin));
}

TEST_CASE("rect_hit: zero-size roi means the whole frame")
{
    // TaskInfo::roi 宽或高为 0 表示全图识别：拿不准，一律要求挪开光标
    REQUIRE(rect_hit(0, 0, 0, 0, 0, 0, 0));
    REQUIRE(rect_hit(1279, 719, 0, 0, 0, 0, 0));
    REQUIRE(rect_hit(-100, -100, 0, 0, 0, 0, 0));
    REQUIRE(rect_hit(100, 100, 0, 0, 100, 0, 0));
    REQUIRE(rect_hit(100, 100, 0, 0, 0, 100, 0));
}

TEST_CASE("same_point: tolerance")
{
    REQUIRE(same_point(10, 20, 10, 20));
    REQUIRE(same_point(10, 20, 10 + DefaultParkTolerance, 20));
    REQUIRE(same_point(10, 20, 10, 20 - DefaultParkTolerance));
    REQUIRE_FALSE(same_point(10, 20, 10 + DefaultParkTolerance + 1, 20));
    REQUIRE_FALSE(same_point(10, 20, 10, 20 + DefaultParkTolerance + 1));

    // 多显示器下光标可能落在负坐标（报告包里出现过 -1519 , 348）
    REQUIRE(same_point(-1519, 348, -1519, 348));
    REQUIRE(same_point(-1519, 348, -1519 + DefaultParkTolerance, 348));
    REQUIRE_FALSE(same_point(-1519, 348, -1519 + DefaultParkTolerance + 1, 348));
}

TEST_CASE("should_restore_cursor: only when nobody moved the cursor")
{
    // 没人碰过 → 精确归位
    REQUIRE(should_restore_cursor(608, 278, 608, 278));
    REQUIRE(should_restore_cursor(608, 278, 608 + DefaultParkTolerance, 278));
    // 用户动过 → 尊重其当前位置，不回拉
    REQUIRE_FALSE(should_restore_cursor(608, 278, 608 + DefaultParkTolerance + 1, 278));
    REQUIRE_FALSE(should_restore_cursor(608, 278, 640, 400));
}

TEST_CASE("should_relocate: tri-state cursor location policy")
{
    // 光标明确在客户区外 → 客户端不会把指针画进画面，不挪（挂机时鼠标停在 MAA 窗口上就是这种）
    REQUIRE_FALSE(should_relocate(CursorLocateState::Outside, false));
    REQUIRE_FALSE(should_relocate(CursorLocateState::Outside, true));

    // 光标在画面内 → 只有确实命中兴趣区才挪
    REQUIRE_FALSE(should_relocate(CursorLocateState::Inside, false));
    REQUIRE(should_relocate(CursorLocateState::Inside, true));

    // 无法判定 → 拿不准就挪（与门控引入前一致）
    REQUIRE(should_relocate(CursorLocateState::Unknown, false));
    REQUIRE(should_relocate(CursorLocateState::Unknown, true));
}

TEST_CASE("to_string: cursor locate state is loggable")
{
    REQUIRE(std::string(to_string(CursorLocateState::Inside)) == "inside");
    REQUIRE(std::string(to_string(CursorLocateState::Outside)) == "outside");
    REQUIRE(std::string(to_string(CursorLocateState::Unknown)) == "unknown");
}
