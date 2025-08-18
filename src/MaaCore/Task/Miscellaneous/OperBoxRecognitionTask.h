#pragma once
#include "Common/AsstBattleDef.h"
#include "Task/AbstractTask.h"
#include "Vision/Miscellaneous/OperBoxImageAnalyzer.h"

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
    void callback_analyze_result(bool done);
    bool swipe_and_analyze();

    std::unordered_map<std::string, OperBoxInfo> m_own_opers;
};
}
