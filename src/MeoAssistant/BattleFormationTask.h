#pragma once
#include "AbstractTask.h"
#include "AsstBattleDef.h"

namespace asst
{
    class BattleFormationTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~BattleFormationTask() override = default;

        void set_stage_name(std::string name);

    protected:
        virtual bool _run() override;

        bool enter_selection_page();
        bool select_opers_in_cur_page();
        void swipe_page();
        bool confirm_selection();

        std::string m_stage_name;
        std::unordered_map<std::string, std::vector<BattleDeployOper>> m_groups;
    };
}
