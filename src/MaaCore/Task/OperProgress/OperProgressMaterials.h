#pragma once

#include <algorithm>
#include <charconv>
#include <optional>
#include <string_view>

namespace asst::oper_progress
{
inline std::optional<int> parse_material_number(std::string_view text)
{
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string_view::npos) {
        return std::nullopt;
    }
    text = text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc {} || end != text.data() + text.size() || value < 0) {
        return std::nullopt;
    }
    return value;
}

struct MaterialCount
{
    int owned = 0;
    int required = 0;

    int missing() const { return std::max(required - owned, 0); }
};

inline std::optional<MaterialCount> parse_material_count(std::string_view text)
{
    const auto slash = text.find('/');
    if (slash == std::string_view::npos) {
        return std::nullopt;
    }
    const auto owned = parse_material_number(text.substr(0, slash));
    const auto required = parse_material_number(text.substr(slash + 1));
    if (!owned || !required || *required == 0) {
        return std::nullopt;
    }
    return MaterialCount { *owned, *required };
}

struct DualChipRequirements : MaterialCount
{
    int chip_packs() const { return missing() * 2; }

    int catalysts() const { return missing(); }
};

inline std::optional<DualChipRequirements> parse_dual_chip_requirements(std::string_view text)
{
    const auto count = parse_material_count(text);
    // 五星、六星精二分别需要三个、四个双芯片；拒绝截断或串入其他材料的 OCR。
    if (!count || (count->required != 3 && count->required != 4)) {
        return std::nullopt;
    }
    return DualChipRequirements { *count };
}

class ChipPackBatch
{
public:
    ChipPackBatch(std::string_view target_item, int required) :
        m_target_item(target_item),
        m_required(required)
    {
    }

    // StageDrops.stats 为当前 FightTask 的累计值；重复回调不能重复计数，其他职业掉落不抵扣缺口。
    void observe(std::string_view item_id, int cumulative_quantity)
    {
        if (item_id == m_target_item && cumulative_quantity >= 0) {
            m_obtained = std::max(m_obtained, std::min(cumulative_quantity, m_required));
        }
    }

    int obtained() const { return m_obtained; }

    int remaining() const { return m_required - m_obtained; }

private:
    std::string_view m_target_item;
    int m_required = 0;
    int m_obtained = 0;
};
}
