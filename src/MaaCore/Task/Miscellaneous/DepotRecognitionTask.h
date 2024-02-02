#pragma once
#include "Task/AbstractTask.h"

#include <unordered_map>

#include "Vision/Miscellaneous/DepotImageAnalyzer.h"

namespace asst
{
    class DepotRecognitionTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~DepotRecognitionTask() noexcept override = default;
        bool set_ordered_item_id(const std::vector<std::string> ids);

    protected:
        virtual bool _run() override;

        bool swipe_and_analyze();
        void callback_analyze_result(bool done);
        void swipe();
        std::unordered_map<std::string, ItemInfo> m_all_items;
        std::vector<std::string> m_ordered_item_id;
    };
}
