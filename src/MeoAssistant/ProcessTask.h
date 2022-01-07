#pragma once

#include "AbstractTask.h"
#include "AsstDef.h"

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

        void set_tasks(std::vector<std::string> tasks_name) noexcept
        {
            m_cur_tasks_name = std::move(tasks_name);
        }
        void set_times_limit(std::string name, int limit)
        {
            m_times_limit.emplace(std::move(name), limit);
        }

    protected:
        virtual bool _run() override;

        void exec_stage_drops();
        void exec_click_task(const Rect& matched_rect);
        void exec_swipe_task(ProcessTaskAction action);

        std::shared_ptr<TaskInfo> m_cur_task_ptr;
        std::vector<std::string> m_cur_tasks_name;
        std::unordered_map<std::string, int> m_rear_delay;
        std::unordered_map<std::string, int> m_times_limit;
        std::unordered_map<std::string, int> m_exec_times;
    };
}
