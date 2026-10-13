#pragma once

#include <charconv>
#include <optional>
#include <string_view>

namespace asst::depot_quantity
{
// Duplicate observations in one read-only scan must agree. Once uncertain,
// later observations cannot silently rehabilitate the first chosen value.
inline bool agrees(int previous, bool previous_verified, int current, bool current_verified)
{
    return previous_verified && current_verified && previous == current;
}

// Rounded unit displays (e.g. 80.5万) are useful in the legacy display but
// cannot establish exact stock for an automated quota decision.
inline std::optional<int> exact_ascii(std::string_view text, int multiplier)
{
    if (multiplier != 1 || text.empty() || text.find_first_not_of("0123456789") != std::string_view::npos) {
        return std::nullopt;
    }
    int quantity = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), quantity);
    if (error != std::errc {} || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return quantity;
}

// Only the positively identified, native LMD detail card is calibrated here.
// Do not turn another item's number, an abbreviation, or a partial label into stock.
inline std::optional<int> lmd_detail(std::string_view name, std::string_view label, std::string_view text)
{
    if (name != "龙门币" || label != "库存") {
        return std::nullopt;
    }
    return exact_ascii(text, 1);
}
}
