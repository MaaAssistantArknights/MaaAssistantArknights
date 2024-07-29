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

    // 考虑到程序共用部分，把actions执行部分抽取出来
    bool do_action(const battle::copilot::Action& action, [[maybe_unused]] size_t index);

    // 阻塞式判断action命令中的前置条件是否满足，阻塞至满足才执行后续的操作
    bool do_action_sync(const battle::copilot::Action& action, [[maybe_unused]] size_t index);

    // 非阻塞式判断action命令中的前置条件是否满足，如果条件满足执行后续的操作，否则返回false
    bool do_action_async(const battle::copilot::Action& action, [[maybe_unused]] size_t index);

    const std::string& get_name_from_group(const std::string& action_name);
    void notify_action(const battle::copilot::Action& action);
    bool wait_operator_ready(const battle::copilot::Action& action);
    void update_image_if_empty(cv::Mat* _Image);
    void do_strategy_and_update_image(cv::Mat* _Image);
    bool wait_condition_not(const battle::copilot::Action& action);       // 等待所有条件都不满足
    bool wait_condition_all(const battle::copilot::Action& action);       // 等待所有条件都满足
    bool wait_condition_any(const battle::copilot::Action& action);       // 等待某个条件都满足
    bool check_condition_not(const battle::copilot::TriggerInfo& action); // 所有条件都不满足
    bool check_condition_all(const battle::copilot::TriggerInfo& action); // 所有条件都满足
    bool check_condition_any(const battle::copilot::TriggerInfo& action); // 某个条件都满足
    bool check_condition(const battle::copilot::TriggerInfo& action);

    bool check_point(const battle::copilot::PointInfo& _Current);
    bool check_point_not(const battle::copilot::PointInfo& _Current);
    bool check_point_all(const battle::copilot::PointInfo& _Current);
    bool check_point_any(const battle::copilot::PointInfo& _Current);

    bool enter_bullet_time(const std::string& name, const Point& location);
    void sleep_and_do_strategy(unsigned millisecond);

    auto gen_snap_shot() -> battle::copilot::PointInfo::SnapShot;
    bool find_snap_shot(const std::string& code) const noexcept;
    void save_snap_shot(const std::string& code);
    auto get_snap_shot(const std::string& code) const noexcept -> battle::copilot::PointInfo::SnapShot const&;

    virtual bool check_in_battle(const cv::Mat& reusable = cv::Mat(), bool weak = true) override;

    battle::copilot::CombatData m_combat_data;
    std::unordered_map</*group*/ std::string, /*oper*/ std::string> m_oper_in_group;
    std::map<std::string, battle::copilot::PointInfo::SnapShot> m_snap_shots; // 在运行过程中存储的快照信息

    bool m_in_bullet_time = false;
    bool m_need_to_wait_until_end = false;
    std::shared_ptr<std::unordered_map<std::string, std::string>> m_formation_ptr = nullptr;
};
}
