#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
    class ProcessTask;
    class ReclamationControlTask;

    class ReclamationTask : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "ReclamationAlgorithm";

        ReclamationTask(const AsstCallback& callback, Assistant* inst);
        virtual ~ReclamationTask() override = default;

        virtual bool set_params(const json::value& params) override;

    private:
        std::shared_ptr<AbstractTask> m_reclamation_task_ptr = nullptr;

        // switch theme to 1 沙中之火
        void init_reclamation_task();

        // switch theme to 2 沙洲遗闻
        void init_reclamation_2_task();
    };
}
