#include "RoguelikeMap.h"

#include <algorithm>
#include <limits>

#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"

asst::RoguelikeMap::RoguelikeMap()
{
    m_curr_pos = create_and_insert_node(RoguelikeNodeType::Init, init_index, 0).value();
}

// ————————————————————————————————————————————————————————————————————————————————
// update map
// ————————————————————————————————————————————————————————————————————————————————

std::optional<size_t>
    asst::RoguelikeMap::create_and_insert_node(const RoguelikeNodeType& type, const size_t& column, const int& y)
{
    const RoguelikeNodePtr node = std::make_shared<RoguelikeNode>(type, column, y);
    return insert_node(node, column);
}

void asst::RoguelikeMap::add_edge(const size_t& source, const size_t& target)
{
    if (source >= m_nodes.size() || target >= m_nodes.size()) {
        Log.error(__FUNCTION__, "| node does not exist");
        return;
    }

    const RoguelikeNodePtr& source_node = m_nodes.at(source);
    const RoguelikeNodePtr& target_node = m_nodes.at(target);
    source_node->succs.emplace_back(target);
    target_node->preds.emplace_back(source);
    Log.info(__FUNCTION__, "| Node", source, "-> Node", target);
}

void asst::RoguelikeMap::set_curr_pos(const size_t& node_index)
{
    if (node_index >= m_nodes.size()) {
        Log.error(__FUNCTION__, "| node does not exist");
        return;
    }

    Log.info(__FUNCTION__, "| move from Node", m_curr_pos, "to Node ", node_index);
    m_curr_pos = node_index;
}

void asst::RoguelikeMap::update_node_costs()
{
    for (const RoguelikeNodePtr& node : ranges::reverse_view(m_nodes)) {
        node->cost = m_cost_fun(node);
        if (!node->succs.empty()) {
            auto succ_costs = node->succs | views::transform([&](const size_t node_index) {
                                  return m_cost_fun(m_nodes.at(node_index));
                              });
            node->cost += ranges::min(succ_costs);
        }
    }
}

void asst::RoguelikeMap::reset()
{
    m_nodes.clear();
    m_column_indices.clear();
    m_curr_pos = create_and_insert_node(RoguelikeNodeType::Init, init_index, 0).value();
}

// ————————————————————————————————————————————————————————————————————————————————
// get map information
// ————————————————————————————————————————————————————————————————————————————————

size_t asst::RoguelikeMap::get_column_begin(const size_t& column) const
{
    if (column == init_index) {
        return init_index;
    }
    if (column >= m_column_indices.size()) {
        Log.warn(__FUNCTION__, "| column does not exist");
        return m_nodes.size();
    }
    return m_column_indices.at(column - 1);
}

size_t asst::RoguelikeMap::get_column_end(const size_t& column) const
{
    if (column >= m_column_indices.size()) {
        Log.warn(__FUNCTION__, "| column does not exist");
        return m_nodes.size();
    }
    return m_column_indices.at(column);
}

size_t asst::RoguelikeMap::get_next_node() const
{
    const RoguelikeNodePtr curr = m_nodes.at(m_curr_pos);

    if (curr->succs.empty()) {
        Log.error(__FUNCTION__, "| no successor nodes");
        return m_curr_pos;
    }

    const size_t next_index = ranges::min(curr->succs, [&](const size_t& node1_index, const size_t& node2_index) {
        return m_nodes.at(node1_index)->cost < m_nodes.at(node2_index)->cost;
    });
    return next_index;
}

// ————————————————————————————————————————————————————————————————————————————————
// get node fields
// ————————————————————————————————————————————————————————————————————————————————

/*
#define DEFINE_GET_NODE_FIELD(RETURN_TYPE, FIELD)                                \
RETURN_TYPE asst::RoguelikeMap::get_node_##FIELD(const size_t& node_index) const \
{                                                                                \
    if (node_index >= m_nodes.size()) {                                          \
        Log.error(__FUNCTION__, "| node does not exist");                        \
    }                                                                            \
                                                                                 \
    return m_nodes.at(node_index)->FIELD;                                        \
}

DEFINE_GET_NODE_FIELD(asst::RoguelikeNodeType, type)
DEFINE_GET_NODE_FIELD(size_t, column)
DEFINE_GET_NODE_FIELD(int, y)
DEFINE_GET_NODE_FIELD(bool, visited)
DEFINE_GET_NODE_FIELD(std::vector<size_t>, succs)
DEFINE_GET_NODE_FIELD(std::vector<size_t>, preds)
DEFINE_GET_NODE_FIELD(int, cost)
DEFINE_GET_NODE_FIELD(int, refresh_times)

#undef DEFINE_GET_NODE_FIELD
*/

asst::RoguelikeNodeType asst::RoguelikeMap::get_node_type(const size_t& node_index) const
{
    if (node_index >= m_nodes.size()) {
        Log.error(__FUNCTION__, "| node does not exist");
    }

    return m_nodes.at(node_index)->type;
}

size_t asst::RoguelikeMap::get_node_column(const size_t& node_index) const
{
    if (node_index >= m_nodes.size()) {
        Log.error(__FUNCTION__, "| node does not exist");
    }

    return m_nodes.at(node_index)->column;
}

int asst::RoguelikeMap::get_node_y(const size_t& node_index) const
{
    if (node_index >= m_nodes.size()) {
        Log.error(__FUNCTION__, "| node does not exist");
    }

    return m_nodes.at(node_index)->y;
}

bool asst::RoguelikeMap::get_node_visited(const size_t& node_index) const
{
    if (node_index >= m_nodes.size()) {
        Log.error(__FUNCTION__, "| node does not exist");
    }

    return m_nodes.at(node_index)->visited;
}

std::vector<size_t> asst::RoguelikeMap::get_node_succs(const size_t& node_index) const
{
    if (node_index >= m_nodes.size()) {
        Log.error(__FUNCTION__, "| node does not exist");
    }

    return m_nodes.at(node_index)->succs;
}

std::vector<size_t> asst::RoguelikeMap::get_node_preds(const size_t& node_index) const
{
    if (node_index >= m_nodes.size()) {
        Log.error(__FUNCTION__, "| node does not exist");
    }

    return m_nodes.at(node_index)->preds;
}

int asst::RoguelikeMap::get_node_cost(const size_t& node_index) const
{
    if (node_index >= m_nodes.size()) {
        Log.error(__FUNCTION__, "| node does not exist");
    }

    return m_nodes.at(node_index)->cost;
}

int asst::RoguelikeMap::get_node_refresh_times(const size_t& node_index) const
{
    if (node_index >= m_nodes.size()) {
        Log.error(__FUNCTION__, "| node does not exist");
    }

    return m_nodes.at(node_index)->refresh_times;
}

// ————————————————————————————————————————————————————————————————————————————————
// set node fields
// ————————————————————————————————————————————————————————————————————————————————

void asst::RoguelikeMap::set_node_type(const size_t& node_index, RoguelikeNodeType type)
{
    m_nodes.at(node_index)->type = type;
}

void asst::RoguelikeMap::set_node_visited(const size_t& node_index, bool visisted)
{
    m_nodes.at(node_index)->visited = visisted;
}

void asst::RoguelikeMap::set_node_refresh_times(const size_t& node_index, int refresh_times)
{
    m_nodes.at(node_index)->refresh_times = refresh_times;
}

// ================================================================================
// private
// ================================================================================

std::optional<size_t> asst::RoguelikeMap::insert_node(const RoguelikeNodePtr& node, const size_t& column)
{
    // 第一个 node 必须为 init node
    if (column != init_index && m_nodes.empty()) [[unlikely]] {
        Log.error(__FUNCTION__, "| insert node to column", column, "before init node");
        return std::nullopt;
    }
    // 只允许有一个 init node
    if (column == init_index && column < m_column_indices.size() && m_column_indices.at(column) > 0) [[unlikely]] {
        Log.error(__FUNCTION__, "| init node has already exist");
        return std::nullopt;
    }
    // 边界鉴定
    if (column > std::numeric_limits<int>::max()) [[unlikely]] { // to avoid narrowing conversion
        Log.error(__FUNCTION__, "| column index", column, "is out of boundary");
        return std::nullopt;
    }

    // 在现有列外插入 node，为其创建新列
    if (column >= m_column_indices.size()) {
        m_column_indices.insert(m_column_indices.end(), column - m_column_indices.size() + 1, m_nodes.size());
    }

    const size_t index = m_column_indices.at(column);

    // 边界鉴定
    if (index > std::numeric_limits<int>::max()) [[unlikely]] { // to avoid narrowing conversion
        Log.error(__FUNCTION__, "| node index", index, "is out of boundary");
        return std::nullopt;
    }

    m_nodes.insert(m_nodes.begin() + static_cast<int>(index), node);
    std::for_each(m_column_indices.begin() + static_cast<int>(column), m_column_indices.end(), [](size_t& x) {
        x += 1;
    });

    return index;
}
