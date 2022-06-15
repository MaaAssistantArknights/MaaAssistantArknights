#pragma once
#include "PackageTask.h"

namespace asst
{
    class DepotTask : public PackageTask
    {
    public:
        using PackageTask::PackageTask;
        virtual ~DepotTask() noexcept = default;

        virtual bool run() override;

        static constexpr const char* TaskType = "Depot";
    protected:
        void swipe();
    };
}
