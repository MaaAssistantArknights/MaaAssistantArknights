#pragma once
#include "Task/InterfaceTask.h"

#include <memory>
#include <meojson/json.hpp>

#include "Common/AsstTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"

namespace asst
{
class MultiCopilotTaskPlugin;
class BattleProcessTask;
class BattleFormationTask;
class ProcessTask;
class ParadoxRecognitionTask;

// 抄作业任务
class CopilotTask final : public InterfaceTask
{
public:
    struct MultiCopilotConfig
    {
        int id = -1;
        std::string filename;                         // 文件名
        std::optional<std::string> nav_name_override; // 关卡名
        bool is_raid = false;                         // 是否是突袭

        MEO_JSONIZATION(MEO_OPT id, filename, MEO_OPT nav_name_override, MEO_OPT is_raid);
    };

public:
    inline static constexpr std::string_view TaskType = "Copilot";

    CopilotTask(const AsstCallback& callback, Assistant* inst);
    virtual ~CopilotTask() override = default;

    virtual bool run() override;
    virtual bool set_params(const json::value& params) override;

    std::string get_stage_name() const { return m_stage_name; }

private:
    // 自动重开以一次执行为单位维护状态：多作业按列表项，单作业按循环轮次。
    enum class StageAttemptResult
    {
        Success,
        RetryAfterLeak,
        RetryAfterFailure,
        Error,
    };

    enum class AutoRestartState
    {
        Enabled,
        Restarting,
        Recovered,
        LimitReached,
    };

    enum class SingleStagePageType
    {
        Unknown,
        StageMap,
        P3StageList,
    };

    struct SingleStageModeSwitch
    {
        std::string text;
        Rect rect;
    };

    std::optional<std::filesystem::path> parse_copilot_filename(const std::string& name);
    bool run_with_auto_restart();
    StageAttemptResult run_stage_attempt(size_t run_index);
    bool cache_single_stage_display_name();
    bool cache_single_stage_mode(const cv::Mat& image);
    bool reopen_single_stage();
    bool restore_single_stage_mode();
    std::optional<SingleStageModeSwitch> detect_single_stage_mode(const cv::Mat& image) const;
    std::optional<Rect> find_stage_on_current_page(const cv::Mat& image, const std::string& stage_name) const;
    bool confirm_single_stage(const cv::Mat& image) const;
    static bool is_stage_code_candidate(const std::string& text);
    static bool stage_text_matches(const std::string& text, const std::string& stage_name);
    void notify_auto_restart(
        AutoRestartState state,
        size_t run_index,
        size_t restart_times = 0,
        StageAttemptResult reason = StageAttemptResult::Success);

    std::shared_ptr<MultiCopilotTaskPlugin> m_multi_copilot_plugin_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_medicine_task_ptr = nullptr;
    std::shared_ptr<BattleFormationTask> m_formation_task_ptr = nullptr;
    std::shared_ptr<BattleProcessTask> m_battle_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_stop_task_ptr = nullptr;
    std::string m_stage_name;
    std::string m_single_stage_display_name;
    std::string m_single_stage_mode;
    SingleStagePageType m_single_stage_page_type = SingleStagePageType::Unknown;
    bool m_has_subtasks_duplicate = false;
    bool m_auto_restart = false;
    size_t m_auto_restart_times = 3;
    size_t m_run_count = 0;
    size_t m_subtasks_per_run = 0;
};
}
