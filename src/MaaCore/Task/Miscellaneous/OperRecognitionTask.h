#pragma once
#include "Common/AsstBattleDef.h"
#include "Task/AbstractTask.h"
#include <unordered_set>

namespace asst
{
    class OperRecognitionTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~OperRecognitionTask() override = default;

    protected:
        struct OperBoxInfo
        {
            std::string name;
        };
        virtual bool _run() override;
        void swipe_page();
        std::vector<OperBoxInfo> analyzer_opers();
        std::vector<OperBoxInfo> own_opers;
        void callback_analyze_result(bool done);
        std::unordered_set<std::string> get_own_oper_names();
    };
}
