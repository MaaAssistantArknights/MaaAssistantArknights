#pragma once

#include <algorithm>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>

namespace asst::oper_progress
{
struct FactoryProductionState
{
    std::string device;
    std::string room;
    std::string product;

    bool matches(std::string_view current_device, std::string_view current_room) const
    {
        return !device.empty() && !room.empty() && device == current_device && room == current_room;
    }
};

inline std::optional<std::string> parse_factory_room(std::string_view text)
{
    std::optional<std::string> room;
    for (size_t i = 0; i + 4 <= text.size(); ++i) {
        const auto candidate = text.substr(i, 4);
        if (candidate[0] != 'B' || candidate[1] < '1' || candidate[1] > '3' || candidate[2] != '0' ||
            candidate[3] < '1' || candidate[3] > '3') {
            continue;
        }
        // 拒绝多房间号及尾随数字，不把截断或串入其他页面的文字当成制造站编号。
        if (room || (i + 4 < text.size() && text[i + 4] >= '0' && text[i + 4] <= '9')) {
            return std::nullopt;
        }
        room = std::string(candidate);
    }
    return room;
}

template <typename Produce, typename Restore, typename Cancelled>
bool manufacture_with_restore(Produce produce, Restore restore, Cancelled cancelled)
{
    if (cancelled()) {
        return false;
    }
    const bool produced = produce();
    // 用户停止时不得继续点击；调用方保留恢复记录，在下一次培养开始前处理。
    if (cancelled()) {
        return false;
    }
    // 生产/确认/收取失败也必须恢复，不能被 produced 的短路求值跳过。
    const bool restored = restore();
    return produced && restored && !cancelled();
}

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
