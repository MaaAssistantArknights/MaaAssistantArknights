#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastControlTask final : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastControlTask() override = default;

        virtual size_t max_num_of_opers() const noexcept override { return 5ULL; }
    private:
        virtual bool _run() override;
    };
}
