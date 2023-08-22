#pragma once

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace asst::algorithm
{
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
     * @return 可行的分配方案, key 为组名, value 为该组分配的干员, 若无可行方案则返回 std::nullopt, 如:\n
     *         {\n
     *         "A": "干员1",\n
     *         "B": "干员2"\n
     *         }
     */
    inline static std::optional<std::unordered_map<std::string, std::string>> get_char_allocation_for_each_group(
        const std::unordered_map<std::string, std::vector<std::string>>& group_list,
        const std::unordered_set<std::string>& char_set)
    {
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

            DancingLinksModel(const size_t& max_node_num, const size_t& max_ans_size)
                : first(max_node_num), size(max_node_num), left(max_node_num), right(max_node_num), up(max_node_num),
                  down(max_node_num), column(max_node_num), row(max_node_num), answer_stack(max_ans_size)
            {}

            void build(const size_t& column_id)
            {
                for (size_t i = 0; i <= column_id; i++) {
                    left[i] = i - 1;
                    right[i] = i + 1;
                    up[i] = down[i] = i;
                }
                left[0] = column_id;
                right[column_id] = 0;
                index = column_id;
            }

            void insert(const size_t& row_id, const size_t& column_id)
            {
                column[++index] = column_id;
                row[index] = row_id;
                ++size[column_id];
                down[index] = down[column_id];
                up[down[column_id]] = index;
                up[index] = column_id;
                down[column_id] = index;
                if (!first[row_id]) {
                    first[row_id] = left[index] = right[index] = index;
                }
                else {
                    right[index] = right[first[row_id]];
                    left[right[first[row_id]]] = index;
                    left[index] = first[row_id];
                    right[first[row_id]] = index;
                }
            }

            bool dance(const size_t& depth)
            {
                if (!right[0]) {
                    answer_stack_size = depth;
                    return true;
                }
                size_t column_id = right[0];
                for (size_t i = right[0]; i != 0; i = right[i]) {
                    if (size[i] < size[column_id]) {
                        column_id = i;
                    }
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

        // 建立结点、组、干员与各自 id 的映射关系
        std::vector<std::pair<std::string, std::string>> node_id_mapping;
        std::vector<std::string> group_id_mapping;
        std::vector<std::string> char_id_mapping;
        std::unordered_map<std::string, size_t> group_name_mapping;
        std::unordered_map<std::string, size_t> char_name_mapping;

        for (auto& i : group_list) {
            group_name_mapping[i.first] = group_id_mapping.size();
            group_id_mapping.emplace_back(i.first);
            bool is_empty = true;
            for (auto& j : i.second) {
                if (char_set.contains(j)) {
                    is_empty = false;
                    node_id_mapping.emplace_back(i.first, j);
                    if (!char_name_mapping.contains(j)) {
                        char_name_mapping[j] = char_id_mapping.size();
                        char_id_mapping.emplace_back(j);
                    }
                }
            }
            if (is_empty) {
                return std::nullopt;
            }
        }

        // 建 01 矩阵
        const size_t node_num = node_id_mapping.size();
        const size_t group_num = group_id_mapping.size();
        const size_t char_num = char_id_mapping.size();

        DancingLinksModel dancing_links_model(2 * node_num + group_num + 2 * char_num + 1, group_num + char_num);

        dancing_links_model.build(group_num + char_num);

        for (size_t i = 0; i < node_num; i++) {
            dancing_links_model.insert(i + 1, group_name_mapping[node_id_mapping[i].first] + 1);
            dancing_links_model.insert(i + 1, group_num + char_name_mapping[node_id_mapping[i].second] + 1);
        }

        for (size_t i = 0; i < char_num; i++) {
            dancing_links_model.insert(i + node_num + 1, i + group_num + 1);
        }

        // dance!!
        bool has_solution = dancing_links_model.dance(0);

        // 判定结果
        if (!has_solution) return std::nullopt;

        std::unordered_map<std::string, std::string> return_value;

        for (size_t i = 0; i < dancing_links_model.answer_stack_size; i++) {
            if (dancing_links_model.answer_stack[i] > node_num) break;
            return_value.insert(node_id_mapping[dancing_links_model.answer_stack[i] - 1]);
        }

        return return_value;
    }
} // namespace asst::algorithm
