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

        void set_support_unit_name(std::string name);

        enum class DataResource
        {
            Copilot,
            SSSCopilot,
        };
        void set_data_resource(DataResource resource);

    protected:
        using OperGroup = std::vector<battle::OperUsage>;

        virtual bool _run() override;

        bool add_additional();

        bool enter_selection_page();
        bool select_opers_in_cur_page(std::vector<OperGroup>& groups);
        void swipe_page();
        bool confirm_selection();
        bool click_role_table(battle::Role role);
        bool parse_formation();
        bool select_random_support_unit();

        std::vector<TextRect> analyzer_opers();

        std::string m_stage_name;
        std::unordered_map<battle::Role, std::vector<OperGroup>> m_formation;
        std::string m_the_right_name;
        std::string m_support_unit_name;
        DataResource m_data_resource = DataResource::Copilot;
        std::vector<AdditionalFormation> m_additional;
    };
}
