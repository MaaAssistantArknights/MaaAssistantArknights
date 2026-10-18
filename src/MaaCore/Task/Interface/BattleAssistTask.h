#pragma once

#include "Task/InterfaceTask.h"
#include "Task/BattleHelper.h"

namespace asst
{
class BattleAssistTask final : public InterfaceTask, private BattleHelper
{
public:
    inline static constexpr std::string_view TaskType = "BattleAssist";

    BattleAssistTask(const AsstCallback& callback, Assistant* inst);
    virtual ~BattleAssistTask() override = default;

    virtual bool set_params(const json::value& params) override;

private:
    virtual bool run() override;
    virtual AbstractTask& this_task() override { return *this; }

    bool m_auto_use_skills = false;
    bool m_auto_speed_up = false;
    int m_skill_scan_interval_ms = 1500;
};
}
