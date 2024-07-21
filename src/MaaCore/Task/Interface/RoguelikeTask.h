#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
    class ProcessTask;
    class RoguelikeConfig;
    class RoguelikeInvestTaskPlugin;
    class RoguelikeDebugTaskPlugin;
    class RoguelikeCustomStartTaskPlugin;

    class RoguelikeTask : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "Roguelike";

        RoguelikeTask(const AsstCallback& callback, Assistant* inst);
        virtual ~RoguelikeTask() override = default;

        virtual bool set_params(const json::value& params) override;

    private:
        std::shared_ptr<ProcessTask> m_roguelike_task_ptr = nullptr;
        std::shared_ptr<RoguelikeConfig> m_config_ptr = nullptr;
        std::shared_ptr<RoguelikeInvestTaskPlugin> m_invest_ptr = nullptr;
        std::shared_ptr<RoguelikeDebugTaskPlugin> m_debug_ptr = nullptr;
        std::shared_ptr<RoguelikeCustomStartTaskPlugin> m_custom_ptr = nullptr;

    };
}
