#pragma once

#include "AbstractRoguelikeMap.h"

#include <array>
#include <optional>
#include <vector>

namespace asst
{
class RoguelikeBoskyPassageMap
{
public:
    RoguelikeBoskyPassageMap();
    ~RoguelikeBoskyPassageMap() = default;

    // ===================== 创建/更新 =====================
    // 创建一个坐标 (x, y) 上的节点（若不存在）。返回其 index。
    // x ∈ [0, WIDTH), y ∈ [0, HEIGHT)
    [[nodiscard]] std::optional<size_t>
        create_and_insert_node(int x, int y, RoguelikeNodeType type = RoguelikeNodeType::Unknown);

    void set_visited(size_t index);

    void set_node_type(size_t index, RoguelikeNodeType type);

    void reset();

    // ===================== 获取信息 =====================
    [[nodiscard]] size_t size() const { return m_existing_count; }

    [[nodiscard]] static constexpr int width() { return WIDTH; }

    [[nodiscard]] static constexpr int height() { return HEIGHT; }

    // 开放但未访问的节点集合
    [[nodiscard]] std::vector<size_t>
        get_open_unvisited_nodes(std::optional<RoguelikeNodeType> type_filter = std::nullopt) const;
    // 全部开放节点（包含已通关）
    [[nodiscard]] std::vector<size_t> get_open_nodes(std::optional<RoguelikeNodeType> type_filter = std::nullopt) const;

    [[nodiscard]] RoguelikeNodeType get_node_type(size_t index) const;
    [[nodiscard]] bool get_node_exists(size_t index) const;
    [[nodiscard]] bool get_node_open(size_t index) const;
    [[nodiscard]] bool get_node_visited(size_t index) const;

    [[nodiscard]] int get_node_x(size_t index) const;
    [[nodiscard]] int get_node_y(size_t index) const;
    // 获得节点像素坐标（左上角）
    [[nodiscard]] std::pair<int, int>
        get_node_pixel(size_t index, int origin_x, int origin_y, int column_offset, int row_offset) const;

    // 返回邻接已存在节点的 index（不存在或未创建的节点不返回）
    [[nodiscard]] std::vector<size_t> get_adjacent_existing_nodes(size_t index) const;

    // 按 (x,y) 访问节点
    [[nodiscard]] std::optional<size_t> coord_to_index(int x, int y) const;

    // 像素 -> 节点；若节点不存在则返回 std::nullopt。
    [[nodiscard]] std::optional<size_t> pixel_to_index(
        int px,
        int py,
        int origin_x,
        int origin_y,
        int column_offset,
        int row_offset,
        int node_width,
        int node_height) const;

    // 像素 -> 节点；若节点不存在则按给定 type 创建
    [[nodiscard]] std::optional<size_t> ensure_node_from_pixel(
        int px,
        int py,
        int origin_x,
        int origin_y,
        int column_offset,
        int row_offset,
        int node_width,
        int node_height,
        bool is_open,
        RoguelikeNodeType type = RoguelikeNodeType::Unknown);

    // 中心节点 index
    static constexpr int CENTER_X = 2;
    static constexpr int CENTER_Y = 3;

    [[nodiscard]] constexpr size_t center_index() const { return static_cast<size_t>(CENTER_Y * WIDTH + CENTER_X); }

private:
    // 固定 5x7 网格；节点总数 ≤ 31
    static constexpr int WIDTH = 5;
    static constexpr int HEIGHT = 7;
    static constexpr size_t MAX_NODES = 31;

    struct Node
    {
        RoguelikeNodeType type = RoguelikeNodeType::Unknown;
        bool exists = false;  // 是否已创建
        bool is_open = false; // 是否开放可探索
        bool visited = false; // 是否已访问
    };

    std::array<Node, WIDTH * HEIGHT> m_nodes {}; // 直接按 y*WIDTH + x 索引
    size_t m_existing_count = 0;

    // 辅助函数：获取有效的节点引用，如果索引无效或节点不存在则返回 nullptr
    [[nodiscard]] Node* get_valid_node(size_t index);

    [[nodiscard]] static constexpr bool in_bounds(int x, int y) { return 0 <= x && x < WIDTH && 0 <= y && y < HEIGHT; }
};
}
