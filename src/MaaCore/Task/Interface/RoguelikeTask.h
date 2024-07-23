#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
    class ProcessTask;
    class RoguelikeConfig;
    class RoguelikeControlTaskPlugin; 
    class RoguelikeInvestTaskPlugin;
    class RoguelikeDebugTaskPlugin;
    class RoguelikeCustomStartTaskPlugin;
    class RoguelikeFoldartalGainTaskPlugin;
    class RoguelikeFoldartalStartTaskPlugin;
    class RoguelikeFoldartalUseTaskPlugin;
    class RoguelikeCollapsalParadigmTaskPlugin;

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
        std::shared_ptr<RoguelikeControlTaskPlugin> m_control_ptr = nullptr;
        std::shared_ptr<RoguelikeInvestTaskPlugin> m_invest_ptr = nullptr;
        std::shared_ptr<RoguelikeDebugTaskPlugin> m_debug_ptr = nullptr;
        std::shared_ptr<RoguelikeCustomStartTaskPlugin> m_custom_ptr = nullptr;
        std::shared_ptr<RoguelikeFoldartalGainTaskPlugin> m_foldartal_gain_ptr = nullptr;
        std::shared_ptr<RoguelikeFoldartalStartTaskPlugin> m_foldartal_start_ptr = nullptr;
        std::shared_ptr<RoguelikeFoldartalUseTaskPlugin> m_foldartal_use_ptr = nullptr;
        std::shared_ptr<RoguelikeCollapsalParadigmTaskPlugin> m_cp_ptr = nullptr;
    };
}
