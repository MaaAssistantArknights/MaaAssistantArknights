#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
    class ProcessTask;
    class RoguelikeConfig;
    class RoguelikeCustomStartTaskPlugin;
    class RoguelikeDebugTaskPlugin;

    class RoguelikeTask : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "Roguelike";

        RoguelikeTask(const AsstCallback& callback, Assistant* inst);
        virtual ~RoguelikeTask() override = default;

        virtual bool set_params(const json::value& params) override;

    private:
        std::shared_ptr<ProcessTask> m_roguelike_task_ptr = nullptr;
        std::shared_ptr<RoguelikeConfig> m_roguelike_config_ptr = nullptr;
        std::shared_ptr<RoguelikeCustomStartTaskPlugin> m_custom_start_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeDebugTaskPlugin> m_debug_plugin_ptr = nullptr;
        void m_roguelike_register_plugins();

    };
}
