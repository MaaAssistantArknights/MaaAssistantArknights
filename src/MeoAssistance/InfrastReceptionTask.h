#pragma once
#include "InfrastProductionTask.h"

namespace asst {
    class InfrastReceptionTask : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastReceptionTask() = default;
        virtual bool run() override;
    private:
        bool swipe_to_the_bottom_of_clue_list_on_the_right();
    };
}
