#pragma once
#include "Task/Miscellaneous/BattleProcessTask.h"

namespace asst
{
class SingleStepBattleProcessTask : public BattleProcessTask
{
public:
    using Actions = std::vector<asst::battle::copilot::Action>;

public:
    using BattleProcessTask::BattleProcessTask;
    virtual ~SingleStepBattleProcessTask() override = default;

    using BattleProcessTask::clear;

    static bool set_stage_name_cache(const std::string& stage_name);
    void set_actions(Actions actions);

protected:
    virtual bool _run() override;

    virtual bool need_to_wait_until_end() const noexcept override { return false; }

private:
    Actions m_actions;

    // for debug
    inline static std::string m_stage_name_cache;
};
}
