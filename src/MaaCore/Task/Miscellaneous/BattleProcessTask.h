#pragma once
#include "Task/AbstractTask.h"
#include "Task/BattleHelper.h"

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/TilePack.h"

namespace asst
{
class BattleProcessTask : public AbstractTask, public BattleHelper
{
public:
    BattleProcessTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
    virtual ~BattleProcessTask() override = default;

    virtual bool set_stage_name(const std::string& stage_name) override;
    void set_wait_until_end(bool wait_until_end);

    // 启用后，一旦识别到战斗中的红色漏怪标记，就在结算前退出战斗。
    void set_abort_on_leak(bool abort_on_leak) noexcept { m_abort_on_leak = abort_on_leak; }

    // CopilotTask 根据本次尝试的漏怪状态，判断是否可以重开同一作业。
    bool has_leaked() const noexcept { return m_has_leaked; }

    void set_formation_task_ptr(std::shared_ptr<std::unordered_map<std::string, std::string>> value);

protected:
    virtual bool _run() override;

    virtual AbstractTask& this_task() override { return *this; }

    virtual void clear() override;
    virtual bool do_strategic_action(const cv::Mat& reusable = cv::Mat()) override;

    virtual bool
        do_derived_action([[maybe_unused]] const battle::copilot::Action& action, [[maybe_unused]] size_t index)
    {
        return false;
    }

    virtual battle::copilot::CombatData& get_combat_data() { return m_combat_data; }

    virtual bool need_to_wait_until_end() const { return m_need_to_wait_until_end; }

    bool to_group();
    bool do_action(const battle::copilot::Action& action, size_t index);

    const std::string& get_name_from_group(const std::string& action_name);
    void notify_action(const battle::copilot::Action& action);
    bool wait_condition(const battle::copilot::Action& action);
    bool enter_bullet_time(const std::string& name, const Point& location);
    void sleep_and_do_strategy(unsigned millisecond);

    // 只有识别到漏怪且放弃战斗流程成功完成后才返回 true。
    bool check_and_abandon_on_leak(const cv::Mat& image);

    battle::copilot::CombatData m_combat_data;
    std::unordered_map</*group*/ std::string, /*oper*/ std::string> m_oper_in_group;

    bool m_in_bullet_time = false;
    bool m_need_to_wait_until_end = false;
    bool m_abort_on_leak = false;
    bool m_has_leaked = false;
    std::shared_ptr<std::unordered_map<std::string, std::string>> m_formation_ptr = nullptr;
};
}
