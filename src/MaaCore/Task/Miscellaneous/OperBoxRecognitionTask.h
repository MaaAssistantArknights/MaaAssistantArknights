#pragma once
#include "Common/AsstBattleDef.h"
#include "Task/AbstractTask.h"
#include "Vision/Oper/OperBoxImageAnalyzer.h"

namespace asst
{
class OperBoxRecognitionTask : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~OperBoxRecognitionTask() override = default;

    void set_paradox_filter(bool enabled) noexcept { m_paradox_filter = enabled; }

    void set_next_only(bool enabled, std::vector<std::string> candidates)
    {
        m_next_only = enabled;
        m_candidates = std::move(candidates);
    }

protected:
    virtual bool _run() override;
    void swipe_page();
    void callback_analyze_result(bool done);
    bool swipe_and_analyze();

    std::unordered_map<std::string, OperBoxInfo> m_own_opers;
    bool m_paradox_filter = false;
    bool m_next_only = false;
    std::vector<std::string> m_candidates;
};
}
