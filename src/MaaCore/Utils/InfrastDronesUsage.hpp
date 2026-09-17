#pragma once

#include <algorithm>
#include <span>
#include <string>
#include <string_view>

namespace asst::infrast
{
// 只有全部贸易站都可靠识别为另一种订单时，才建议用户检查无人机用途。
inline bool is_trade_drones_usage_mismatched(std::string_view usage, std::span<const std::string> products)
{
    if (products.empty() || (usage != "Money" && usage != "SyntheticJade")) {
        return false;
    }
    const std::string_view opposite_product = usage == "Money" ? "SyntheticJade" : "Money";
    return std::ranges::all_of(products, [opposite_product](const auto& product) {
        return product == opposite_product;
    });
}
}
