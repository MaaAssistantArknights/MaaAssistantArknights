#pragma once
#include "PackageTask.h"

namespace asst
{
    class DepotTask final : public PackageTask
    {
    public:
        DepotTask(AsstCallback callback, void* callback_arg);
        virtual ~DepotTask() = default;

        static constexpr const char* TaskType = "Depot";
    private:
    };
}
