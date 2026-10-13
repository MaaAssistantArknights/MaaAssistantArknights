#pragma once
#include <array>
#include <string>
#include <string_view>

namespace asst::depot_token
{
// Native 1280x720 material grid: the final row's measured ROI ends at y=642.
// The storage button starts at y=646. Do not admit its pixels or another layout.
inline bool grid_geometry(int frame_width, int frame_height, int x, int y, int width, int height)
{
    return frame_width == 1280 && frame_height == 720 && x >= 0 && x < 1280 && y >= 90 && y < 642 && width > 0 &&
           height > 0 && width <= 1280 - x && height <= 642 - y;
}

// A Chinese native read-only detail card is positive boundary evidence.
// A failed template, a token-looking icon or an arbitrary mention of potential
// is not sufficient. Both the operator name and its exact usage must agree.
inline bool
    detail(std::string_view name, std::string_view usage, std::string_view stock_label, std::string_view storage_label)
{
    constexpr std::string_view suffix = "的信物";
    if (stock_label != "库存") {
        return false;
    }
    // Native royal class tokens have no storage button. The calibrated card
    // says e.g. 近卫皇家信物 / 用于提升★6近卫干员的潜能 / 采购中心.
    constexpr std::array<std::string_view, 8> classes = {
        "先锋", "近卫", "重装", "狙击", "术师", "医疗", "辅助", "特种"
    };
    for (const auto profession : classes) {
        if (name == std::string(profession) + "皇家信物") {
            const std::string expected = "用于提升★6" + std::string(profession) + "干员的潜能";
            return storage_label == "采购中心" && (usage == expected || usage == expected + "。");
        }
    }
    if (!name.ends_with(suffix) || name.size() <= suffix.size() || storage_label != "存入保险库") {
        return false;
    }
    const std::string expected = "用于提升" + std::string(name.substr(0, name.size() - suffix.size())) + "的潜能";
    return usage == expected || usage == expected + "。";
}
}
