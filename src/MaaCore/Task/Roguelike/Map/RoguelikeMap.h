#pragma once

#include "AbstractRoguelikeMap.h"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace asst
{
struct RoguelikeNode
{
    RoguelikeNodeType type = RoguelikeNodeType::Unknown;
    const size_t column = 0;
    const int y = 0;
    bool visited = false;
    std::vector<size_t> succs; // successors 在 RoguelikeMap().m_nodes 中的 index
    std::vector<size_t> preds; // predecessors 在 RoguelikeMap().m_nodes 中的 index
    int cost = 0;
    // –––––––– 萨卡兹主题引入 ––––––––––––––––––––––––
    int refresh_times = 0;

    RoguelikeNode(const RoguelikeNodeType _t, const size_t _c, const int _y) :
        type(_t),
        column(_c),
        y(_y)
    {
    }
};

using RoguelikeNodePtr = std::shared_ptr<RoguelikeNode>;
using RoguelikeNodeCostFun = std::function<int(const RoguelikeNodePtr&)>;

// ================================================================================
// Q: RoguelikeMap 应该支持哪些功能？
// A: 原则上我希望 RoguelikeNode 和 RoguelikeNodePtr 仅在RoguelikeMap 内部调用
//    外部操作对节点的识别以 index 为准, 因此 public 成员函数往往应当接受 size_t 类型
//    的 node_index 参数
// ================================================================================
class RoguelikeMap
{
public:
    RoguelikeMap();

    // ———————— update map ————————————————————————————————————————————————————————————
    // 创建并插入一个 node，返回新 node 的 index
    std::optional<size_t> create_and_insert_node(const RoguelikeNodeType& type, const size_t& column, const int& y);

    void add_edge(const size_t& source, const size_t& target);
    void set_curr_pos(const size_t& node_index);

    void set_cost_fun(const RoguelikeNodeCostFun& cost_fun) { m_cost_fun = cost_fun; }

    void update_node_costs();
    void reset();

    // ———————— get map details ———————————————————————————————————————————————————————
    [[nodiscard]] size_t size() const { return m_nodes.size(); }

    [[nodiscard]] size_t get_num_columns() const { return m_column_indices.size(); }

    [[nodiscard]] size_t get_curr_pos() const { return m_curr_pos; }

    [[nodiscard]] size_t get_column_begin(const size_t& column) const;
    [[nodiscard]] size_t get_column_end(const size_t& column) const;
    [[nodiscard]] size_t get_next_node() const;

    // ———————— get node fields ———————————————————————————————————————————————————————
    [[nodiscard]] RoguelikeNodeType get_node_type(const size_t& node_index) const;
    [[nodiscard]] size_t get_node_column(const size_t& node_index) const;
    [[nodiscard]] int get_node_y(const size_t& node_index) const;
    [[nodiscard]] bool get_node_visited(const size_t& node_index) const;
    [[nodiscard]] std::vector<size_t> get_node_succs(const size_t& node_index) const;
    [[nodiscard]] std::vector<size_t> get_node_preds(const size_t& node_index) const;
    [[nodiscard]] int get_node_cost(const size_t& node_index) const;
    [[nodiscard]] int get_node_refresh_times(const size_t& node_index) const;

    // ———————— set node fields ———————————————————————————————————————————————————————
    void set_node_type(const size_t& node_index, RoguelikeNodeType type);
    void set_node_visited(const size_t& node_index, bool visisted);
    void set_node_refresh_times(const size_t& node_index, int refresh_times);

    // ———————— constants and variables ———————————————————————————————————————————————
    static constexpr size_t INIT_INDEX = 0; // 常量，既是 init 的 node index 也是它的 column index

private:
    // ———————— update map ————————————————————————————————————————————————————————————
    std::optional<size_t> insert_node(const RoguelikeNodePtr& node, const size_t& column);

    // ———————— constants and variables ———————————————————————————————————————————————
    std::vector<RoguelikeNodePtr> m_nodes;
    std::vector<size_t> m_column_indices; // m_column_indices[c] 代表列 c 的 node index 的上限 (exclusive)
    size_t m_curr_pos = 0;                // 当前位置的 node index
    RoguelikeNodeCostFun m_cost_fun = [&]([[maybe_unused]] const RoguelikeNodePtr& node) {
        return 0;
    };
};
}
