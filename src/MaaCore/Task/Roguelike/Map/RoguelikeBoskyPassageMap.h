#pragma once

#include "AbstractRoguelikeMap.h"
#include "Utils/SingletonHolder.hpp"

#include <array>
#include <optional>
#include <vector>

namespace asst
{

enum class RoguelikeBoskySubNodeType
{
    Unknown = 0,
    Ling = 1, // 令 - 常乐 掷地有声
    Shu = 2,  // 黍 - 常乐 种因得果
    Nian = 3, // 年 - 常乐 三缺一
};

class RoguelikeBoskyPassageMap : public SingletonHolder<RoguelikeBoskyPassageMap>
{
public:
    ~RoguelikeBoskyPassageMap() = default;

    // ===================== 创建/更新 =====================
    // 创建一个坐标 (x, y) 上的节点（若不存在）。返回其 index。
    // x ∈ [0, WIDTH), y ∈ [0, HEIGHT)
    [[nodiscard]] std::optional<size_t>
        create_and_insert_node(int x, int y, RoguelikeNodeType type = RoguelikeNodeType::Unknown, bool is_open = true);

    void set_visited(size_t index);

    void set_node_type(size_t index, RoguelikeNodeType type);

    void set_curr_pos(size_t index) { m_curr_pos = index; }

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

    [[nodiscard]] size_t get_curr_pos() const { return m_curr_pos; }

    [[nodiscard]] RoguelikeNodeType get_node_type(size_t index) const;
    [[nodiscard]] bool get_node_exists(size_t index) const;
    [[nodiscard]] bool get_node_open(size_t index) const;
    [[nodiscard]] bool get_node_visited(size_t index) const;

    [[nodiscard]] int get_node_x(size_t index) const;
    [[nodiscard]] int get_node_y(size_t index) const;
    // 获得节点像素坐标（左上角）
    [[nodiscard]] std::pair<int, int>
        get_node_pixel(size_t index, int origin_x, int origin_y, int column_offset, int row_offset) const;

    // 按 (x,y) 访问节点
    [[nodiscard]] std::optional<size_t> coord_to_index(int x, int y) const;

    // 地图配置参数结构体
    struct BoskyPassageMapConfig
    {
        int origin_x = 0;
        int origin_y = 0;
        int middle_x = 0;
        int middle_y = 0;
        int last_x = 0;
        int last_y = 0;
        int node_width = 0;
        int node_height = 0;
        int column_offset = 0;
        int row_offset = 0;
        int roi_margin = 0;

        BoskyPassageMapConfig() = default;

        BoskyPassageMapConfig(std::vector<int> special_params)
        {
            if (special_params.size() >= 11) {
                origin_x = special_params.at(0);
                origin_y = special_params.at(1);
                middle_x = special_params.at(2);
                middle_y = special_params.at(3);
                last_x = special_params.at(4);
                last_y = special_params.at(5);
                node_width = special_params.at(6);
                node_height = special_params.at(7);
                column_offset = special_params.at(8);
                row_offset = special_params.at(9);
                roi_margin = special_params.at(10);
            }
        }
    };

    // 像素 -> 网格；返回 (x, y) 坐标
    [[nodiscard]] std::pair<int, int> pixel_to_grid_coords(int px, int py, const BoskyPassageMapConfig& config) const;

    // 像素 -> 节点；若节点不存在则返回 std::nullopt。
    [[nodiscard]] std::optional<size_t> pixel_to_index(int px, int py, const BoskyPassageMapConfig& config) const;

    // 像素 -> 节点；若节点不存在则按给定 type 创建
    [[nodiscard]] std::optional<size_t> ensure_node_from_pixel(
        int px,
        int py,
        const BoskyPassageMapConfig& config,
        bool is_open,
        RoguelikeNodeType type = RoguelikeNodeType::Unknown);

    // 设置子类型
    void set_node_subtype(size_t index, RoguelikeBoskySubNodeType sub_type);

    // 获取子类型
    [[nodiscard]] RoguelikeBoskySubNodeType get_node_subtype(size_t index) const;

    // 设置目标子类型
    void set_target_subtype(RoguelikeBoskySubNodeType target) { m_target_subtype = target; }

    // 获取目标子类型
    [[nodiscard]] RoguelikeBoskySubNodeType get_target_subtype() const { return m_target_subtype; }

    // 中心节点 index
    static constexpr int CENTER_X = 3;
    static constexpr int CENTER_Y = 2;

    [[nodiscard]] constexpr size_t center_index() const { return static_cast<size_t>(CENTER_Y * WIDTH + CENTER_X); }

private:
    // 固定 5x7 网格；节点总数 ≤ 31
    static constexpr int WIDTH = 7;
    static constexpr int HEIGHT = 5;
    static constexpr size_t MAX_NODES = 31;

    struct Node
    {
        RoguelikeNodeType type = RoguelikeNodeType::Unknown;
        RoguelikeBoskySubNodeType sub_type = RoguelikeBoskySubNodeType::Unknown;
        bool exists = false;  // 是否已创建
        bool is_open = false; // 是否开放可探索
        bool visited = false; // 是否已访问
    };

    std::array<Node, WIDTH * HEIGHT> m_nodes {}; // 直接按 y*WIDTH + x 索引
    size_t m_curr_pos = 0;
    size_t m_existing_count = 0;
    RoguelikeBoskySubNodeType m_target_subtype = RoguelikeBoskySubNodeType::Unknown;

    // 辅助函数：获取有效的节点引用，如果索引无效或节点不存在则返回 nullptr
    [[nodiscard]] Node* get_valid_node(size_t index);

    [[nodiscard]] static constexpr bool in_bounds(int x, int y) { return 0 <= x && x < WIDTH && 0 <= y && y < HEIGHT; }
};
}
