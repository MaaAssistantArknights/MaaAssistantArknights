#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Common/AsstTypes.h"
#include "Config/Roguelike/RoguelikeMapConfig.h"

namespace asst
{
// 黑流树海（DrowningSeekers）每层是一个四邻接网格迷宫，与列式 DAG 的 RoguelikeMap 不同。
// 本模型每次识别后由 Analyzer 结果重建（非单例、非持久化），提供网格坐标、邻接与带权最短路查询。
class RoguelikeDrowningSeekersMap
{
public:
    // 节点在网格中的成分类型
    enum class CellKind
    {
        None = 0, // 空格（无节点）
        Road,     // 路点（无铭牌）
        Object,   // 事件节点（有铭牌，类型经 OCR 识别）
    };

    struct Cell
    {
        CellKind kind = CellKind::None;
        RoguelikeNodeType type = RoguelikeNodeType::Unknown; // 仅 Object 有意义
        Point center;                                        // 屏幕像素坐标（节点中心）
        bool visited = false;                                // 铭牌前有绿色 ✓
    };

    // 代价函数：给定一个格的坐标返回进入该格的代价（越大越不愿走）
    using CostFun = std::function<int(int col, int row, const Cell&)>;

    RoguelikeDrowningSeekersMap() = default;

    // ———————— 构建 ————————
    void reset();
    // 设置网格尺寸（列数、行数）
    void set_dimensions(int cols, int rows);
    // 写入一个格
    void set_cell(int col, int row, const Cell& cell);
    // 添加一条无向边（两端格坐标）
    void add_edge(int c1, int r1, int c2, int r2);
    // 设置玩家所在格
    void set_player(int col, int row) { m_player = { col, row }; }

    // ———————— 查询 ————————
    [[nodiscard]] int cols() const { return m_cols; }

    [[nodiscard]] int rows() const { return m_rows; }

    [[nodiscard]] bool in_bounds(int col, int row) const
    {
        return 0 <= col && col < m_cols && 0 <= row && row < m_rows;
    }

    [[nodiscard]] const Cell& cell(int col, int row) const;
    [[nodiscard]] bool has_node(int col, int row) const;
    [[nodiscard]] std::pair<int, int> player() const { return m_player; }

    // 某格的四邻接节点（存在边且目标格有节点）
    [[nodiscard]] std::vector<std::pair<int, int>> neighbors(int col, int row) const;

    // 全部有节点的格坐标
    [[nodiscard]] std::vector<std::pair<int, int>> all_nodes() const;

    // 指定类型的、未访问的节点坐标
    [[nodiscard]] std::vector<std::pair<int, int>>
        find_nodes(RoguelikeNodeType type, bool exclude_visited = true) const;

    // ———————— 路径 ————————
    // 从玩家位置到 target 的最短路（按 cost_fun 累加进入代价），返回途径格坐标序列（含起点玩家格与 target）。
    // 不可达返回空。
    [[nodiscard]] std::vector<std::pair<int, int>>
        shortest_path(int target_col, int target_row, const CostFun& cost_fun) const;

    // 便捷：从玩家到 target 的总进入代价（不含玩家格自身），不可达返回 INT_MAX
    [[nodiscard]] int path_cost(int target_col, int target_row, const CostFun& cost_fun) const;

private:
    [[nodiscard]] size_t index(int col, int row) const { return static_cast<size_t>(row) * m_cols + col; }

    int m_cols = 0;
    int m_rows = 0;
    std::vector<Cell> m_cells;                                        // 按 row*cols+col 索引
    std::unordered_map<size_t, std::vector<size_t>> m_adjacency;      // index -> 邻接 index 列表
    std::pair<int, int> m_player = { -1, -1 };
    Cell m_none_cell;                                                 // 越界/空格返回的哨兵
};
}
