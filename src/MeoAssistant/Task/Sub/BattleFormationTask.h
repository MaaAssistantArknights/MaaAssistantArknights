#pragma once
#include "AbstractTask.h"
#include "Utils/AsstBattleDef.h"

namespace asst
{
    class BattleFormationTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~BattleFormationTask() override = default;

        void set_stage_name(std::string name);

    protected:
        using OperGroup = std::vector<BattleDeployOper>;
        virtual bool _run() override;

        bool enter_selection_page();
        bool select_opers_in_cur_page(std::vector<OperGroup>& groups);
        void swipe_page();
        bool confirm_selection();
        bool click_role_table(BattleRole role);
        bool parse_formation();

        std::string m_stage_name;
        std::unordered_map<BattleRole, std::vector<OperGroup>> m_formation;
    };
}
