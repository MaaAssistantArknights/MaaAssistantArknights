#pragma once
#include "Common/AsstBattleDef.h"
#include "Task/AbstractTask.h"

namespace asst
{
    class BattleFormationTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~BattleFormationTask() override = default;

        enum class Filter
        {
            None,
            Trust,
            Cost,
        };
        struct AdditionalFormation
        {
            Filter filter = Filter::None;
            bool double_click_filter = true;
            battle::RoleCounts role_counts;
        };
        void append_additional_formation(AdditionalFormation formation);

        void set_support_unit(std::pair<battle::Role, std::vector<battle::OperUsage>> support_unit);
        void set_support_unit_name(std::string name);
        // 是否追加自定干员
        void set_add_user_additional(bool add_user_additional);
        // 设置追加自定干员列表
        void set_user_additional(std::vector<std::pair<std::string, int>> user_additional);
        // 是否追加低信赖干员
        void set_add_trust(bool add_trust);

        enum class DataResource
        {
            Copilot,
            SSSCopilot,
        };
        void set_data_resource(DataResource resource);

    protected:
        using OperGroup = std::vector<battle::OperUsage>;

        virtual bool _run() override;
        // 根据Role和OperGroup选择干员，并返回不在box里的干员
        std::vector<OperGroup> add_formation(battle::Role role, std::vector<OperGroup> oper_groups);
        // 追加附加干员（按部署费用等小分类）
        bool add_additional();
        // 补充刷信赖的干员，从最小的开始
        bool add_trust_operators();
        bool enter_selection_page();
        bool enter_support_page();
        bool select_opers_in_cur_page(std::vector<OperGroup>& groups);
        void swipe_page();
        void swipe_to_the_left(int times = 2);
        bool confirm_selection();
        bool click_role_table(battle::Role role);
        bool click_support_role_table();
        bool parse_formation();
        bool select_random_support_unit();
        bool select_support_oper();

        void quit(std::string msg = "");

        std::vector<TextRect> analyzer_opers();
        std::vector<TextRect> analyzer_support_opers();

        std::string m_stage_name;
        std::unordered_map<battle::Role, std::vector<OperGroup>> m_formation;
        std::vector<OperGroup> m_user_formation;
        // 编队中干员个数
        int m_size_of_operators_in_formation = 0;
        // 编队中的干员名称，用来判断能不能追加某个干员
        std::set<std::string> m_operators_in_formation;
        bool m_add_user_additional = false;
        // 追加干员表，从头往后加
        std::vector<std::pair<std::string, int>> m_user_additional;
        // 是否需要追加信赖干员
        bool m_add_trust = false;
        // 是否使用自动助战
        bool m_use_support = true;

        DataResource m_data_resource = DataResource::Copilot;
        std::vector<AdditionalFormation> m_additional;
        std::string m_last_oper_name;
        std::pair<battle::Role, OperGroup> m_support_unit;
        size_t m_unselected_cnt = 0;

        std::string m_support_unit_name;
    };
} // namespace asst
