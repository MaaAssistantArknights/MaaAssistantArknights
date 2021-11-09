#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastReceptionTask : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastReceptionTask() = default;
        virtual bool run() override;

        const static std::string FacilityName;

    private:
        bool harvest_clue();
        bool proc_clue();
        bool proc_vacancy();
        bool shift();
        bool swipe_to_the_bottom_of_clue_list_on_the_right();
    };
}
