#include "BlackFlowInventoryModel.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <utility>

namespace asst::blackflow
{
InventoryModel::InventoryModel(InventoryLayout layout) :
    m_layout(std::move(layout))
{
}

void InventoryModel::clear()
{
    m_cells.clear();
}

void InventoryModel::put(InventorySlot slot, InventoryCell cell)
{
    m_cells.insert_or_assign(slot, std::move(cell));
}

bool InventoryModel::contains(InventorySlot slot) const
{
    return m_cells.contains(slot);
}

const InventoryCell* InventoryModel::find(InventorySlot slot) const
{
    const auto found = m_cells.find(slot);
    return found == m_cells.end() ? nullptr : &found->second;
}

int InventoryModel::max_column() const
{
    if (m_cells.empty()) {
        return -1;
    }
    return std::prev(m_cells.end())->first.first;
}

int InventoryModel::flatten(InventorySlot slot) const
{
    return slot.first * m_layout.rows_per_column + slot.second;
}

InventorySlot InventoryModel::unflatten(int index) const
{
    return { index / m_layout.rows_per_column, index % m_layout.rows_per_column };
}

std::optional<InventorySlot> InventoryModel::shifted_by_removal(InventorySlot slot, InventorySlot removed) const
{
    const int index = flatten(slot);
    const int gone = flatten(removed);
    if (index == gone) {
        return std::nullopt;
    }
    return unflatten(index > gone ? index - 1 : index);
}

void InventoryModel::collapse(InventorySlot removed)
{
    std::map<InventorySlot, InventoryCell> shifted;
    for (auto& [slot, cell] : m_cells) {
        const auto moved = shifted_by_removal(slot, removed);
        if (!moved.has_value()) {
            continue;
        }
        shifted.emplace(*moved, std::move(cell));
    }
    m_cells = std::move(shifted);
}

int InventoryModel::row_of(int center_y) const
{
    const double offset = static_cast<double>(center_y - m_layout.first_row_center_y);
    return static_cast<int>(std::lround(offset / static_cast<double>(m_layout.row_pitch)));
}

int InventoryModel::column_of(int absolute_x, int base_x) const
{
    const double offset = static_cast<double>(absolute_x - base_x);
    return static_cast<int>(std::lround(offset / static_cast<double>(m_layout.column_pitch)));
}

std::vector<std::vector<VisibleScrap>> InventoryModel::group_columns(std::vector<VisibleScrap> visible) const
{
    std::ranges::sort(visible, {}, &VisibleScrap::center_x);
    std::vector<std::vector<VisibleScrap>> groups;
    for (VisibleScrap& scrap : visible) {
        if (!groups.empty() && scrap.center_x - groups.back().back().center_x < m_layout.column_pitch / 2) {
            groups.back().emplace_back(std::move(scrap));
        }
        else {
            groups.push_back({ std::move(scrap) });
        }
    }
    return groups;
}

std::optional<std::vector<int>>
    InventoryModel::resolve_columns(std::vector<VisibleScrap>& visible, std::optional<int> hint) const
{
    auto groups = group_columns(visible);
    if (groups.empty() || groups.size() > 2) {
        return std::nullopt;
    }

    std::unordered_map<int, std::unordered_map<int, std::string>> fingerprints;
    for (const auto& [slot, cell] : m_cells) {
        fingerprints[slot.first].emplace(slot.second, cell.name);
    }

    // 用一列中的三条名字反查该列在模型中的编号；屏上同时可见相邻两列时，两列一并比对并要求列号连续。
    const auto match = [&](const std::vector<VisibleScrap>& group) {
        std::vector<std::pair<int, int>> hits;
        for (const auto& [column, rows] : fingerprints) {
            bool consistent = true;
            for (const VisibleScrap& scrap : group) {
                const auto row = rows.find(scrap.row);
                if (row == rows.end() || row->second != scrap.name) {
                    consistent = false;
                    break;
                }
            }
            if (consistent) {
                hits.emplace_back(column, static_cast<int>(group.size()));
            }
        }
        return hits;
    };

    const auto closeness = [&hint](int column) {
        return hint.has_value() ? -std::abs(column - *hint) : 0;
    };

    std::optional<std::pair<std::pair<int, int>, std::vector<int>>> best;
    bool ambiguous = false;
    const auto consider = [&best, &ambiguous](std::pair<int, int> score, std::vector<int> columns) {
        if (!best.has_value() || score > best->first) {
            best = { score, std::move(columns) };
            ambiguous = false;
        }
        else if (score == best->first && columns != best->second) {
            // 匹配数和 closeness 都相同时，不让 unordered_map 的遍历顺序决定当前位置。
            ambiguous = true;
        }
    };

    if (groups.size() == 1) {
        for (const auto& [column, matched] : match(groups[0])) {
            consider({ matched, closeness(column) }, { column });
        }
    }
    else {
        const auto left = match(groups[0]);
        const auto right = match(groups[1]);
        for (const auto& [left_column, left_matched] : left) {
            for (const auto& [right_column, right_matched] : right) {
                if (right_column != left_column + 1) {
                    continue;
                }
                consider({ left_matched + right_matched, closeness(left_column) }, { left_column, right_column });
            }
        }
    }

    if (!best.has_value() || ambiguous) {
        return std::nullopt;
    }

    visible.clear();
    for (std::size_t index = 0; index < groups.size(); ++index) {
        for (VisibleScrap& scrap : groups[index]) {
            scrap.column = best->second[index];
            visible.emplace_back(std::move(scrap));
        }
    }
    return best->second;
}

std::vector<InventorySlot>
    InventoryModel::ranked_candidates(std::size_t discard_max_rank, const std::set<InventorySlot>& excluded) const
{
    std::vector<InventorySlot> candidates;
    for (const auto& [slot, cell] : m_cells) {
        if (cell.rank >= discard_max_rank || excluded.contains(slot)) {
            continue;
        }
        candidates.emplace_back(slot);
    }
    // rank 相同就挑最靠右的：扫描停在最右端，靠右的回走最短。
    std::ranges::sort(candidates, [this](InventorySlot lhs, InventorySlot rhs) {
        const InventoryCell& lhs_cell = m_cells.at(lhs);
        const InventoryCell& rhs_cell = m_cells.at(rhs);
        if (lhs_cell.rank != rhs_cell.rank) {
            return lhs_cell.rank < rhs_cell.rank;
        }
        if (lhs.first != rhs.first) {
            return lhs.first > rhs.first;
        }
        return lhs.second < rhs.second;
    });
    return candidates;
}
}
