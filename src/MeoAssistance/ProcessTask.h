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

        virtual void set_tasks(std::vector<std::string> tasks_name) noexcept
        {
            m_cur_tasks_name = std::move(tasks_name);
        }

    protected:
        virtual bool _run() override;
        bool delay_random();
        void exec_click_task(const Rect& matched_rect);
        void exec_swipe_task(ProcessTaskAction action);

        std::vector<std::string> m_cur_tasks_name;
    };
}
