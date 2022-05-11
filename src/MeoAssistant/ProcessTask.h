#pragma once

#include "AbstractTask.h"
#include "AsstTypes.h"

namespace asst
{
    // 流程任务，按照配置文件里的设置的流程运行
    class ProcessTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        ProcessTask(const AbstractTask& abs, std::vector<std::string> tasks_name);
        ProcessTask(AbstractTask&& abs, std::vector<std::string> tasks_name) noexcept;

        virtual ~ProcessTask() = default;

        virtual bool run() override;

        ProcessTask& set_task_delay(int delay) noexcept;
        ProcessTask& set_tasks(std::vector<std::string> tasks_name) noexcept;
        ProcessTask& set_times_limit(std::string name, int limit);
        ProcessTask& set_rear_delay(std::string name, int delay);

    protected:
        virtual bool _run() override;

        void exec_click_task(const Rect& matched_rect);
        void exec_swipe_task(ProcessTaskAction action);
        void exec_slowly_swipe_task(ProcessTaskAction action);

        std::vector<std::string> m_raw_tasks_name;
        std::vector<std::string> m_cur_tasks_name;
        std::unordered_map<std::string, int> m_rear_delay;
        std::unordered_map<std::string, int> m_times_limit;
        std::unordered_map<std::string, int> m_exec_times;
        static constexpr int TaskDelayUnsetted = -1;
        int m_task_delay = TaskDelayUnsetted;
    };
}
