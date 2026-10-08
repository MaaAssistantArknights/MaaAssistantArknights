#pragma once
#include "Task/InterfaceTask.h"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace asst
{
class FightTimesTaskPlugin;
class ProcessTask;
class StageDropsTaskPlugin;
class StageNavigationTask;
class DrGrandetTaskPlugin;
class SideStoryReopenTask;
class MedicineCounterTaskPlugin;

class FightTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "Fight";

    enum class StopReason
    {
        TargetReached,
        SanityInsufficient,
        DeadlineReached,
        NavigationFailed,
        AutoDeployUnavailable,
        AutoDeployFailed,
        DropRecognitionFailed,
        Cancelled,
        Completed,
        Unknown,
    };

    struct RunResult
    {
        StopReason reason = StopReason::Unknown;
        int medicine_used = 0;
        bool medicine_usage_known = false;
    };

    FightTask(const AsstCallback& callback, Assistant* inst);
    virtual ~FightTask() override = default;

    virtual bool run() override;
    virtual bool set_params(const json::value& params) override;

    // 补料任务使用独立 Fight 实例；识别失败时停止，避免未知掉落导致无限刷取。
    void set_refill_mode(bool enabled = true);

    // 截止后不再开始下一场，已经开始的战斗正常结束。
    void set_valid_until(std::chrono::system_clock::time_point deadline) { m_valid_until = deadline; }

    const RunResult& get_result() const noexcept { return m_result; }

protected:
    std::shared_ptr<ProcessTask> m_start_up_task_ptr = nullptr;
    std::shared_ptr<StageNavigationTask> m_stage_navigation_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_fight_task_ptr = nullptr;
    std::shared_ptr<FightTimesTaskPlugin> m_fight_times_prt = nullptr;
    std::shared_ptr<MedicineCounterTaskPlugin> m_medicine_plugin = nullptr;
    std::shared_ptr<StageDropsTaskPlugin> m_stage_drops_plugin_ptr = nullptr;
    std::shared_ptr<DrGrandetTaskPlugin> m_dr_grandet_task_plugin_ptr = nullptr;
    std::shared_ptr<SideStoryReopenTask> m_sidestory_reopen_task_ptr = nullptr;

private:
    enum class Phase
    {
        Navigation,
        Fight,
    };

    void observe_callback(AsstMsg msg, const json::value& details, Phase phase);
    bool deadline_reached(std::chrono::milliseconds delay = {}) const;

    RunResult m_result;
    std::optional<std::chrono::system_clock::time_point> m_valid_until;
    Phase m_last_phase = Phase::Navigation;
    bool m_refill_mode = false;
    bool m_execution_failed = false;
    bool m_recovery_exhausted = false;
    bool m_battle_start_attempted = false;
    bool m_recovery_attempted = false;
};
}
