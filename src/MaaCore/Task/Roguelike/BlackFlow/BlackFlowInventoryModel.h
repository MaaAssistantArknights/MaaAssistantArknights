#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "BlackFlowPolicy.h"

#include "Common/AsstTypes.h"

namespace asst::blackflow
{
using InventorySlot = std::pair<int, int>;

struct InventoryCell
{
    std::string name;
    std::size_t rank = 0;
};

struct VisibleScrap
{
    std::string name;
    Rect rect;
    double score = 0.0;
    std::size_t rank = 0;
    int center_x = 0;
    int row = 0;
    int column = 0;
};

class InventoryModel
{
public:
    InventoryModel() = default;
    explicit InventoryModel(InventoryLayout layout);

    [[nodiscard]] const InventoryLayout& layout() const noexcept { return m_layout; }

    void clear();
    void put(InventorySlot slot, InventoryCell cell);
    [[nodiscard]] bool contains(InventorySlot slot) const;
    [[nodiscard]] const InventoryCell* find(InventorySlot slot) const;

    [[nodiscard]] std::size_t size() const noexcept { return m_cells.size(); }

    [[nodiscard]] bool empty() const noexcept { return m_cells.empty(); }

    [[nodiscard]] int max_column() const;

    [[nodiscard]] const std::map<InventorySlot, InventoryCell>& cells() const noexcept { return m_cells; }

    [[nodiscard]] int flatten(InventorySlot slot) const;
    [[nodiscard]] InventorySlot unflatten(int index) const;
    [[nodiscard]] std::optional<InventorySlot> shifted_by_removal(InventorySlot slot, InventorySlot removed) const;
    void collapse(InventorySlot removed);

    [[nodiscard]] int row_of(int center_y) const;
    [[nodiscard]] int column_of(int absolute_x, int base_x) const;

    [[nodiscard]] std::vector<std::vector<VisibleScrap>> group_columns(std::vector<VisibleScrap> visible) const;
    [[nodiscard]] std::optional<std::vector<int>>
        resolve_columns(std::vector<VisibleScrap>& visible, std::optional<int> hint) const;

    [[nodiscard]] std::vector<InventorySlot>
        ranked_candidates(std::size_t discard_max_rank, const std::set<InventorySlot>& excluded) const;

private:
    InventoryLayout m_layout;
    std::map<InventorySlot, InventoryCell> m_cells;
};
}
