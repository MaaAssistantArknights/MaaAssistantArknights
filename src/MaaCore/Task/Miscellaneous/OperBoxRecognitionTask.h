#pragma once
#include "Common/AsstBattleDef.h"
#include "Task/AbstractTask.h"
#include "Vision/Miscellaneous/OperBoxImageAnalyzer.h"
#include <unordered_set>

namespace asst
{
    class OperBoxRecognitionTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~OperBoxRecognitionTask() override = default;

    protected:
        virtual bool _run() override;
        void swipe_page();
        std::vector<OperBoxInfo> own_opers;
        void callback_analyze_result(bool done);
        std::unordered_set<std::string> get_own_oper_names();
        bool swipe_and_analyze();
    };
}
