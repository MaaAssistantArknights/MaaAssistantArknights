#include "RoguelikeMap.h"
#include <ranges>
#include <limits>

#include "Utils/Logger.hpp"

asst::RoguelikeMap::RoguelikeMap() {
    create_and_insert_node(RoguelikeNodeType::Init, 0, 0);
}

std::optional<std::size_t> asst::RoguelikeMap::create_and_insert_node(const RoguelikeNodeType& type,
                                                                      const std::size_t& column,
                                                                      const int& y) {
    const RoguelikeNodePtr node = std::make_shared<RoguelikeNode>(type, column, y);
    return insert_node(node, column);
}

bool asst::RoguelikeMap::add_edge(const std::size_t& source, const std::size_t& target) {
    if (source >= m_nodes.size() || target >= m_nodes.size()) {
        return false;
    }
    const RoguelikeNodePtr& source_node = m_nodes.at(source);
    const RoguelikeNodePtr& target_node = m_nodes.at(target);
    source_node->successors.emplace_back(target_node);
    target_node->predecessors.emplace_back(source_node);
    Log.info("RoguelikeMap | ", "Add Edge:", source, target);

    return true;
}

void asst::RoguelikeMap::update_costs() {
    LogTraceFunction;

    for (const RoguelikeNodePtr& node : std::ranges::reverse_view(m_nodes)) {
        if (node->type == RoguelikeNodeType::CombatOps ||
            node->type == RoguelikeNodeType::EmergencyOps ||
            node->type == RoguelikeNodeType::DreadfulFoe) {
            node->cost += 1;
            if (node->refresh_times) {
                node->cost += 999;
            }
        }
        if (!node->successors.empty()) {
            auto succ_costs = node->successors | std::views::transform([&](const RoguelikeNodePtr& n) {
                return n->cost + (n->column == node->column ? 999 : 0); // 暂时不纵向移动
            });
            node->cost += std::ranges::min(succ_costs);
        }
    }
}

std::size_t asst::RoguelikeMap::nspf(const std::size_t& curr_index) {
    LogTraceFunction;

    update_costs();
    const RoguelikeNodePtr curr = m_nodes.at(curr_index);
    const RoguelikeNodePtr next = std::ranges::min(curr->successors,
        [](const RoguelikeNodePtr& n1, const RoguelikeNodePtr& n2) { return n1->cost < n2->cost; });
    return std::distance(m_nodes.begin(), std::ranges::find(m_nodes, next));
}

void asst::RoguelikeMap::clear() {
    m_nodes.clear();
    m_column_indices.clear();
    create_and_insert_node(RoguelikeNodeType::Init, 0, 0);
}

std::size_t asst::RoguelikeMap::size() const {
    return m_nodes.size();
}

std::size_t asst::RoguelikeMap::get_column_begin(const std::size_t& column) const {
    if (column == 0) {
        return 0;
    }
    if (column >= m_column_indices.size()) {
        return m_nodes.size();
    }
    return m_column_indices.at(column - 1);
}

std::size_t asst::RoguelikeMap::get_column_end(const std::size_t& column) const {
    if (column >= m_column_indices.size()) {
        return m_nodes.size();
    }
    return m_column_indices.at(column);
}

asst::RoguelikeNodeType asst::RoguelikeMap::get_node_type(const std::size_t& node_index) const {
    return m_nodes.at(node_index)->type;
}

std::size_t asst::RoguelikeMap::get_node_column(const std::size_t& node_index) const {
    return m_nodes.at(node_index)->column;
}

int asst::RoguelikeMap::get_node_y(const std::size_t& node_index) const {
    return m_nodes.at(node_index)->y;
}

int asst::RoguelikeMap::get_node_cost(const std::size_t& node_index) const {
    return m_nodes.at(node_index)->cost;
}

int asst::RoguelikeMap::get_node_refresh_times(const std::size_t& node_index) const {
    return m_nodes.at(node_index)->refresh_times;
}

std::optional<size_t> asst::RoguelikeMap::find_column(const std::size_t& node_index) const {
    for (std::size_t column = 0; column < m_column_indices.size(); ++column) {
        if (m_column_indices.at(column) > node_index) {
            return column;
        }
    }
    return std::nullopt;
}

void asst::RoguelikeMap::set_node_type(const std::size_t& node_index, RoguelikeNodeType type) {
    m_nodes.at(node_index)->type = type;
}

void asst::RoguelikeMap::set_node_refresh_times(const std::size_t& node_index, int refresh_times) {
    m_nodes.at(node_index)->refresh_times = refresh_times;
}

std::optional<std::size_t> asst::RoguelikeMap::insert_node(const RoguelikeNodePtr& node, const std::size_t& column) {
    if (column == init_index && !m_column_indices.empty() && m_column_indices.at(init_index) >= 1) {
        return std::nullopt;
    }

    if (column > std::numeric_limits<int>::max()) {
        return std::nullopt;
    }

    if (column >= m_column_indices.size()) {
        m_column_indices.insert(m_column_indices.end(),
                                column - m_column_indices.size() + 1,
                                m_nodes.size());
    }

    const std::size_t index = m_column_indices.at(column);
    if (index > std::numeric_limits<int>::max()) { // to avoid narrowing conversion
        return std::nullopt;
    }

    m_nodes.insert(m_nodes.begin() +  static_cast<int>(index), node);
    std::for_each(m_column_indices.begin() + static_cast<int>(column), m_column_indices.end(),
        [](std::size_t& x) { x += 1; });
    m_column_indices.at(column) = index + 1;

    return index;
}
