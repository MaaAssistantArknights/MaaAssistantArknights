#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastReceptionTask final : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastReceptionTask() = default;

        const static std::string FacilityName;

    protected:
        virtual bool _run() override;

    private:
        bool close_end_prompt();
        bool harvest_clue();
        bool proc_clue();
        bool proc_vacancy();
        bool send_clue();
        bool shift();
        bool swipe_to_the_bottom_of_clue_list_on_the_right();
    };
}
