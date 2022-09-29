#pragma once
#include "Task/PackageTask.h"

namespace asst
{
    class ProcessTask;

    class PipelineTask final : public PackageTask
    {
    public:
        PipelineTask(const AsstCallback& callback, void* callback_arg);
        virtual ~PipelineTask() override = default;

        virtual bool set_params(const json::value& params) override;

        static constexpr const char* TaskType = "Pipeline";

    private:
        std::vector<std::string> m_task_name;
        std::shared_ptr<ProcessTask> m_task_ptr;
    };
}
