#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastReceptionTask final : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastReceptionTask() override = default;

    protected:
        virtual bool _run() override;

    private:
        bool close_end_of_clue_exchange();
        bool get_clue();
        bool use_clue();
        bool proc_clue_vacancy();
        bool unlock_clue_exchange();
        bool send_clue();
        bool shift();
    };
}
