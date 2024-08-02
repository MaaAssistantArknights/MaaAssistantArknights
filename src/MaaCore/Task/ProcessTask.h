#pragma once

#include "AbstractTask.h"
#include "Common/AsstTypes.h"
#include "Utils/NoWarningCVMat.h"

namespace asst
{
// 流程任务，按照配置文件里的设置的流程运行
class ProcessTask final : public AbstractTask
{
public:
    enum class TimesLimitType
    {
        Pre,
        Post,
    };

    struct TimesLimitData
    {
        int times = 0;
        TimesLimitType type = TimesLimitType::Pre;
    };

public:
    using AbstractTask::AbstractTask;
    ProcessTask(const AbstractTask& abs, std::vector<std::string> tasks_name);
    ProcessTask(AbstractTask&& abs, std::vector<std::string> tasks_name) noexcept;

    virtual ~ProcessTask() override = default;

    virtual bool run() override;

    ProcessTask& set_task_delay(int delay) noexcept;
    ProcessTask& set_tasks(std::vector<std::string> tasks_name) noexcept;
    ProcessTask& set_times_limit(std::string name, int limit, TimesLimitType type = TimesLimitType::Pre);
    ProcessTask& set_post_delay(std::string name, int delay);
    ProcessTask& set_reusable_image(const cv::Mat& reusable);

    const std::string& get_last_task_name() const noexcept { return m_last_task_name; }

protected:
    enum class NodeStatus
    {
        Success,
        RetryFailed, // 重试次数达到上限，对应 on_error_next
        Runout,      // 执行次数达到上限，对应 exceeded_next
        Interrupted,
        InternalError,
    };

    struct HitDetail
    {
        Rect rect;
        json::value reco_detail;
        TaskConstPtr task_ptr = nullptr;
    };

    virtual bool _run() override;
    virtual json::value basic_info() const override;

    HitDetail find_first(const TaskList& list);
    NodeStatus run_action(const HitDetail& hits) const;
    NodeStatus run_task(const HitDetail& hits);
    std::pair<NodeStatus, TaskConstPtr> find_and_run_task(const TaskList& list);

    TimesLimitData calc_time_limit(TaskConstPtr task) const;
    int calc_post_delay(TaskConstPtr task) const;

    void exec_click_task(const Rect& matched_rect) const;
    void exec_swipe_task(
        const Rect& r1,
        const Rect& r2,
        int duration,
        bool extra_swipe,
        double slope_in,
        double slope_out) const;

    TaskList m_begin_task_list;
    std::string m_pre_task_name;
    std::string m_last_task_name;
    std::unordered_map<std::string, int> m_post_delay;
    std::unordered_map<std::string, TimesLimitData> m_times_limit;
    std::unordered_map<std::string, int> m_exec_times;
    static constexpr int TaskDelayUnsetted = -1;
    int m_task_delay = TaskDelayUnsetted;
    cv::Mat m_reusable;
};
}
