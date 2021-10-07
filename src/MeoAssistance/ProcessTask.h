#pragma once

#include "AbstractTask.h"
#include "AsstDef.h"

namespace asst {
    // 流程任务，按照配置文件里的设置的流程运行
    class ProcessTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~ProcessTask() = default;

        virtual bool run() override;

        virtual void set_tasks(const std::vector<std::string>& cur_tasks_name) {
            m_cur_tasks_name = cur_tasks_name;
        }

    protected:
        bool delay_random();
        void exec_click_task(const Rect& matched_rect);
        void exec_swipe_task(ProcessTaskAction action);

        std::vector<std::string> m_cur_tasks_name;
    };
}