#pragma once

#include "Common/AsstTypes.h"

namespace asst
{
    struct RoguelikeNode;
    using RoguelikeNodePtr = std::shared_ptr<RoguelikeNode>;

    enum class RoguelikeNodeType {
        Init                = -1, // Init
        Unknown             = 0,  // 未知
        CombatOps           = 1,  // 作战
        EmergencyOps        = 2,  // 紧急作战
        DreadfulFoe         = 3,  // 险路恶敌
        Encounter           = 4,  // 不期而遇
        Boons               = 5,  // 古堡馈赠/得偿所愿
        SafeHouse           = 6,  // 安全的角落
        Recreation          = 7,  // 幕间余兴/兴致盎然
        RogueTrader         = 8,  // 诡意行商
        // –––––––– 水月主题引入 ––––––––––––––––––––––––
        RegionalCommissions = 9,  // 地区委托
        LostAndFound        = 10, // 风雨际会/失与得
        Scout               = 11, // 紧急运输/先行一步
        BoskyPassage        = 12, // 误入奇境/树篱之途
        // –––––––– 萨米主题引入 ––––––––––––––––––––––––
        Prophecy            = 13, // 命运所指
        MysteriousPresage   = 14, // 诡秘的预感
        FerociousPresage    = 15, // 凶戾的预感
        // –––––––– 萨卡兹主题引入 ––––––––––––––––––––––––
        IdeaFilter          = 16, // 去违存真
        FaceOff             = 17  // 狭路相逢
    };

    struct RoguelikeNode : public std::enable_shared_from_this<RoguelikeNode> {
        RoguelikeNodeType type = RoguelikeNodeType::Unknown;
        std::size_t column = 0;
        int y = 0;
        bool visited = false;
        std::vector<RoguelikeNodePtr> predecessors;
        std::vector<RoguelikeNodePtr> successors;
        int cost = 0;
        // –––––––– 萨卡兹主题引入 ––––––––––––––––––––––––
        int refresh_times = 0;

        RoguelikeNode(const RoguelikeNodeType _t, const std::size_t _c, const int _y)
            : type(_t), column(_c), y(_y) {};
    };

    class RoguelikeMap
    {
    public:
        RoguelikeMap();

        std::optional<std::size_t> create_and_insert_node(const RoguelikeNodeType& type,
                                                          const std::size_t& column,
                                                          const int& y);
        bool add_edge(const std::size_t& source, const std::size_t& target);
        [[nodiscard]] std::size_t nspf(const std::size_t& curr_index);
        void clear();

        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] std::size_t get_column_begin(const std::size_t& column) const;
        [[nodiscard]] std::size_t get_column_end(const std::size_t& column) const;
        [[nodiscard]] RoguelikeNodeType get_node_type(const std::size_t& node_index) const;
        [[nodiscard]] std::size_t get_node_column(const std::size_t& node_index) const;
        [[nodiscard]] int get_node_y(const std::size_t& node_index) const;
        [[nodiscard]] int get_node_cost(const std::size_t& node_index) const;
        [[nodiscard]] int get_node_refresh_times(const std::size_t& node_index) const;
        [[nodiscard]] std::optional<std::size_t> find_column(const std::size_t& node_index) const;

        void set_node_type(const std::size_t& node_index, RoguelikeNodeType type);
        void set_node_refresh_times(const std::size_t& node_index, int refresh_times);

        const std::size_t init_index = 0;

    private:
        std::optional<std::size_t> insert_node(const RoguelikeNodePtr& node, const std::size_t& column);
        void update_costs();

        std::vector<RoguelikeNodePtr> m_nodes;
        std::vector<std::size_t> m_column_indices;
    };
}
