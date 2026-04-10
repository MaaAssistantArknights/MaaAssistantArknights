#pragma once

#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace asst::algorithm
{
enum class CharAllocationStatus
{
    Success,
    NoSolution,
    Overflow,
    InternalError,
};

struct CharAllocationResult
{
    CharAllocationStatus status { CharAllocationStatus::NoSolution };
    std::unordered_map<std::string, std::string> allocation;

    [[nodiscard]] static CharAllocationResult success(std::unordered_map<std::string, std::string>&& allocation_value)
    {
        return { CharAllocationStatus::Success, std::move(allocation_value) };
    }

    [[nodiscard]] static CharAllocationResult no_solution()
    {
        return { CharAllocationStatus::NoSolution, {} };
    }

    [[nodiscard]] static CharAllocationResult overflow()
    {
        return { CharAllocationStatus::Overflow, {} };
    }

    [[nodiscard]] static CharAllocationResult internal_error()
    {
        return { CharAllocationStatus::InternalError, {} };
    }

    [[nodiscard]] bool has_value() const noexcept
    {
        return status == CharAllocationStatus::Success;
    }
};

/**
 * @brief 根据传入的分组规则及干员列表, 求解一个可行的分配方案
 * @param group_list 分组规则, key 为组名, value 为组内干员列表, 如:\n
 *                   {\n
 *                   "A": {"干员1", "干员2"},\n
 *                   "B": {"干员2", "干员3"}\n
 *                   }
 * @param char_set 干员列表, 如:\n
 *                 {\n
 *                 "干员1",\n
 *                 "干员2"\n
 *                 }
 * @return 求解结果。status 为 CharAllocationStatus::Success 时，allocation 为可行的分配方案，如:\n
 *         {\n
 *         "A": "干员1",\n
 *         "B": "干员2"\n
 *         }\n
 *         status 为 CharAllocationStatus::NoSolution 时表示无可行方案；
 *         其余状态表示求解过程中发生溢出或内部错误。
 */
[[nodiscard]] inline static CharAllocationResult get_char_allocation_for_each_group(
    const std::unordered_map<std::string, std::vector<std::string>>& group_list,
    const std::unordered_set<std::string>& char_set)
{
    if (group_list.empty()) {
        return CharAllocationResult::success(std::unordered_map<std::string, std::string> {});
    }
    if (char_set.empty()) {
        return CharAllocationResult::no_solution();
    }

    const auto checked_add = [](const size_t lhs, const size_t rhs) -> std::optional<size_t> {
        if (lhs > std::numeric_limits<size_t>::max() - rhs) {
            return std::nullopt;
        }
        return lhs + rhs;
    };
    const auto checked_mul = [](const size_t lhs, const size_t rhs) -> std::optional<size_t> {
        if (lhs != 0 && rhs > std::numeric_limits<size_t>::max() / lhs) {
            return std::nullopt;
        }
        return lhs * rhs;
    };

    /*
     * * dlx 算法简介
     *
     * https://oi-wiki.org/search/dlx/
     *
     *
     * * dlx 算法作用
     *
     * 在形如:
     * a: 10010
     * b: 01110
     * c: 01001
     * d: 00100
     * e: 11010
     * 这样的数据里,
     * dlx 可以找到 {a, c, d} 这样每列恰好出现且仅出现一次 1 的数据,
     * 也即对全集的一个精确覆盖:
     * a: 10010
     * c: 01001
     * d: 00100
     *    11111
     *
     *
     * * dlx 算法建模
     *
     * dlx 的列分为 [组号] [干员号] 两部分
     * dlx 的行分为 [可能的选择对] [不选择该干员] 两部分
     *
     * [可能的选择对]:
     * 每行对应一种可能的选择,
     * 将组号，干员号对应位置的列设为1
     *
     * [不选择该干员]:
     * 每行对应不选择某干员的情况,
     * 将干员号对应位置的列设为1
     *
     *
     * * dlx 建模示例
     *
     * 有以下分组:
     * a: {1, 3, 4}
     * b: {2, 3, 5}
     * c: {1, 2, 3}
     * 拥有的干员:
     * {1, 2, 4, 5, 6}
     *
     * 先处理出所有可能的情况:
     * a: {1, 4}
     * b: {2, 5}
     * c: {1, 2}
     *
     * 构造表:
     *   abc 1245
     * 1 100 1000 <a, 1>
     * 2 100 0010 <a, 4>
     * 3 010 0100 <b, 2>
     * 4 010 0001 <b, 5>
     * 5 001 1000 <c, 1>
     * 6 001 0100 <c, 2>
     * 7 000 1000 ~1
     * 9 000 0100 ~2
     * 9 000 0010 ~4
     * A 000 0001 ~5
     *
     * 使用dlx求得一组解:
     * 一个可能的结果是:
     * 行号 {2, 3, 5, A}
     * 即 {<a, 4>, <b, 2>, <c, 1>, ~5}
     *
     * 输出分组结果:
     * a: 4
     * b: 2
     * c: 1
     *
     */

    // dlx 算法模板类
    class DancingLinksModel
    {
    private:
        size_t index {};
        std::vector<size_t> first, size;
        std::vector<size_t> left, right, up, down;
        std::vector<size_t> column, row;

        void remove(const size_t& column_id)
        {
            left[right[column_id]] = left[column_id];
            right[left[column_id]] = right[column_id];
            for (size_t i = down[column_id]; i != column_id; i = down[i]) {
                for (size_t j = right[i]; j != i; j = right[j]) {
                    up[down[j]] = up[j];
                    down[up[j]] = down[j];
                    --size[column[j]];
                }
            }
        }

        void recover(const size_t& column_id)
        {
            for (size_t i = up[column_id]; i != column_id; i = up[i]) {
                for (size_t j = left[i]; j != i; j = left[j]) {
                    up[down[j]] = down[up[j]] = j;
                    ++size[column[j]];
                }
            }
            left[right[column_id]] = right[left[column_id]] = column_id;
        }

    public:
        size_t answer_stack_size {};
        std::vector<size_t> answer_stack;

        DancingLinksModel(const size_t node_capacity, const size_t row_capacity, const size_t column_capacity,
                          const size_t max_ans_size) :
            first(row_capacity, 0),
            size(column_capacity, 0),
            left(node_capacity, 0),
            right(node_capacity, 0),
            up(node_capacity, 0),
            down(node_capacity, 0),
            column(node_capacity, 0),
            row(node_capacity, 0),
            answer_stack(max_ans_size, 0)
        {
        }

        bool build(const size_t column_count)
        {
            if (column_count >= size.size() || column_count >= left.size()) {
                return false;
            }

            left[0] = column_count;
            right[0] = column_count == 0 ? 0 : 1;
            up[0] = down[0] = 0;

            for (size_t i = 1; i <= column_count; ++i) {
                left[i] = i - 1;
                right[i] = (i == column_count) ? 0 : (i + 1);
                up[i] = down[i] = i;
                column[i] = i;
            }
            index = column_count;
            return true;
        }

        bool insert(const size_t row_id, const size_t column_id)
        {
            if (row_id == 0 || row_id >= first.size() || column_id == 0 || column_id >= size.size()
                || index + 1 >= column.size()) {
                return false;
            }

            const size_t node_id = ++index;
            column[node_id] = column_id;
            row[node_id] = row_id;
            ++size[column_id];
            down[node_id] = down[column_id];
            up[down[column_id]] = node_id;
            up[node_id] = column_id;
            down[column_id] = node_id;
            if (first[row_id] == 0) {
                first[row_id] = node_id;
                left[node_id] = right[node_id] = node_id;
            }
            else {
                const size_t first_node = first[row_id];
                right[node_id] = right[first_node];
                left[right[first_node]] = node_id;
                left[node_id] = first_node;
                right[first_node] = node_id;
            }
            return true;
        }

        bool dance(const size_t depth)
        {
            if (!right[0]) {
                answer_stack_size = depth;
                return true;
            }

            if (depth >= answer_stack.size()) {
                return false;
            }

            size_t column_id = right[0];
            for (size_t i = right[0]; i != 0; i = right[i]) {
                if (size[i] < size[column_id]) {
                    column_id = i;
                }
            }
            if (size[column_id] == 0) {
                return false;
            }
            remove(column_id);
            for (size_t i = down[column_id]; i != column_id; i = down[i]) {
                answer_stack[depth] = row[i];
                for (size_t j = right[i]; j != i; j = right[j]) {
                    remove(column[j]);
                }
                if (dance(depth + 1)) {
                    return true;
                }
                for (size_t j = left[i]; j != i; j = left[j]) {
                    recover(column[j]);
                }
            }
            recover(column_id);
            return false;
        }
    };

    struct CandidateNode
    {
        size_t group_id {};
        size_t char_id {};
    };

    size_t candidate_upper_bound = 0;
    for (const auto& [_, candidates] : group_list) {
        const auto next_candidate_upper_bound = checked_add(candidate_upper_bound, candidates.size());
        if (!next_candidate_upper_bound) {
            return CharAllocationResult::overflow();
        }
        candidate_upper_bound = *next_candidate_upper_bound;
    }

    // 建立结点、组、干员与各自 id 的映射关系
    std::vector<CandidateNode> node_id_mapping;
    std::vector<std::string> group_id_mapping;
    std::vector<std::string> char_id_mapping;
    std::unordered_map<std::string, size_t> char_name_mapping;

    node_id_mapping.reserve(candidate_upper_bound);
    group_id_mapping.reserve(group_list.size());
    char_id_mapping.reserve(char_set.size());
    char_name_mapping.reserve(char_set.size());

    for (const auto& [group_name, candidates] : group_list) {
        const size_t group_id = group_id_mapping.size();
        group_id_mapping.emplace_back(group_name);

        bool has_candidate = false;
        std::unordered_set<std::string> seen_candidates;
        seen_candidates.reserve(candidates.size());

        for (const auto& candidate_name : candidates) {
            if (!char_set.contains(candidate_name) || !seen_candidates.emplace(candidate_name).second) {
                continue;
            }

            has_candidate = true;
            const auto [char_it, inserted] = char_name_mapping.try_emplace(candidate_name, char_id_mapping.size());
            if (inserted) {
                char_id_mapping.emplace_back(candidate_name);
            }
            node_id_mapping.emplace_back(group_id, char_it->second);
        }

        if (!has_candidate) {
            return CharAllocationResult::no_solution();
        }
    }

    // 建 01 矩阵
    const size_t node_num = node_id_mapping.size();
    const size_t group_num = group_id_mapping.size();
    const size_t char_num = char_id_mapping.size();

    if (char_num < group_num) {
        return CharAllocationResult::no_solution();
    }

    const auto doubled_node_num = checked_mul(node_num, 2);
    const auto column_num = checked_add(group_num, char_num);
    const auto row_num = checked_add(node_num, char_num);
    if (!doubled_node_num || !column_num || !row_num) {
        return CharAllocationResult::overflow();
    }
    const auto data_node_num = checked_add(*doubled_node_num, char_num);
    if (!data_node_num) {
        return CharAllocationResult::overflow();
    }
    const auto max_node_index = checked_add(*column_num, *data_node_num);
    if (!max_node_index) {
        return CharAllocationResult::overflow();
    }
    const auto node_capacity = checked_add(*max_node_index, 1);
    const auto row_capacity = checked_add(*row_num, 1);
    const auto column_capacity = checked_add(*column_num, 1);
    if (!node_capacity || !row_capacity || !column_capacity) {
        return CharAllocationResult::overflow();
    }

    DancingLinksModel dancing_links_model(*node_capacity, *row_capacity, *column_capacity, char_num);

    if (!dancing_links_model.build(*column_num)) {
        return CharAllocationResult::internal_error();
    }

    for (size_t i = 0; i < node_num; i++) {
        const auto& node = node_id_mapping[i];
        if (!dancing_links_model.insert(i + 1, node.group_id + 1)
            || !dancing_links_model.insert(i + 1, group_num + node.char_id + 1)) {
            return CharAllocationResult::internal_error();
        }
    }

    for (size_t i = 0; i < char_num; i++) {
        if (!dancing_links_model.insert(i + node_num + 1, i + group_num + 1)) {
            return CharAllocationResult::internal_error();
        }
    }

    // dance!!
    bool has_solution = dancing_links_model.dance(0);

    // 判定结果
    if (!has_solution) {
        return CharAllocationResult::no_solution();
    }

    std::unordered_map<std::string, std::string> return_value;
    return_value.reserve(group_num);

    for (size_t i = 0; i < dancing_links_model.answer_stack_size; i++) {
        if (dancing_links_model.answer_stack[i] > node_num) {
            continue;
        }
        const auto& node = node_id_mapping[dancing_links_model.answer_stack[i] - 1];
        return_value.emplace(group_id_mapping[node.group_id], char_id_mapping[node.char_id]);
    }

    return CharAllocationResult::success(std::move(return_value));
}
} // namespace asst::algorithm
