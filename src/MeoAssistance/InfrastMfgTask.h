#pragma once
#include "InfrastShiftTask.h"

namespace asst {
    class InfrastMfgTask : public InfrastShiftTask
    {
    public:
        using InfrastShiftTask::InfrastShiftTask;
        virtual ~InfrastMfgTask() = default;
        virtual bool run() override;

        static const std::string FacilityName;

    private:
    };
}
