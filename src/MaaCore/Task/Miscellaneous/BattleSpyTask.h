#pragma once
#include "Task/AbstractTask.h"
#include "Task/BattleHelper.h"

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/TilePack.h"

namespace asst
{
class BattleSpyTask : public AbstractTask, public BattleHelper
{
public:
    BattleSpyTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
    virtual ~BattleSpyTask() override = default;

    virtual bool set_stage_name(const std::string& stage_name) override;
    void set_wait_until_end(bool wait_until_end);
    void set_formation_task_ptr(std::shared_ptr<std::unordered_map<std::string, std::string>> value);

protected:
    virtual bool _run() override;

    virtual AbstractTask& this_task() override { return *this; }

    virtual void clear() override;

    virtual bool
        do_derived_action([[maybe_unused]] const battle::copilot::Action& action, [[maybe_unused]] size_t index)
    {
        return false;
    }

    virtual battle::copilot::CombatData& get_combat_data() { return m_combat_data; }

    virtual bool need_to_wait_until_end() const { return m_need_to_wait_until_end; }

    bool to_group();

    void notify_action(const json::value& value);
    bool is_skill_ready(const cv::Mat& imageRect);

    battle::copilot::CombatData m_combat_data;
    std::unordered_map</*group*/ std::string, /*oper*/ std::string> m_oper_in_group;

    bool m_in_bullet_time = false;
    bool m_need_to_wait_until_end = false;
    std::shared_ptr<std::unordered_map<std::string, std::string>> m_formation_ptr = nullptr;
};
}
