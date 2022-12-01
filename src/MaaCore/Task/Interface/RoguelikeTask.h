#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
    class ProcessTask;
    class RoguelikeRecruitTaskPlugin;
    class RoguelikeSkillSelectionTaskPlugin;
    class RoguelikeBattleTaskPlugin;
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
        std::shared_ptr<RoguelikeRecruitTaskPlugin> m_recruit_task_ptr = nullptr;
        std::shared_ptr<RoguelikeSkillSelectionTaskPlugin> m_skill_task_ptr = nullptr;
        std::shared_ptr<RoguelikeBattleTaskPlugin> m_battle_task_ptr = nullptr;
        std::shared_ptr<RoguelikeCustomStartTaskPlugin> m_custom_start_task_ptr = nullptr;
        std::shared_ptr<RoguelikeDebugTaskPlugin> m_debug_task_ptr = nullptr;
    };
}
