#pragma once
#include "Task/Miscellaneous/BattleProcessTask.h"

namespace asst
{
    class SingleStepBattleProcessTask : public BattleProcessTask
    {
    public:
        using Actions = std::vector<asst::battle::copilot::Action>;
        
    public:
        using BattleProcessTask::BattleProcessTask;
        virtual ~SingleStepBattleProcessTask() override = default;

        void set_actions(Actions actions);

    protected:
        virtual bool _run() override;
        virtual bool need_to_wait_until_end() const noexcept override { return false; }

    private:
        Actions m_actions;
    };
}
