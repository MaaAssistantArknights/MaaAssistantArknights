#pragma once
#include "PackageTask.h"

#include <unordered_map>

#include "DepotImageAnalyzer.h"

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
        bool swipe_and_analyze();
        void callback_analyze_result();
        void swipe();
        std::unordered_map<std::string, ItemInfo> m_all_items;
    };
}
