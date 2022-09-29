#pragma once
#include "PackageTask.h"

namespace asst
{
    class ProcessTask;
    class RoguelikeRecruitTaskPlugin;
    class RoguelikeSkillSelectionTaskPlugin;
    class RoguelikeBattleTaskPlugin;
    class RoguelikeCustomStartTaskPlugin;

    class RoguelikeTask : public PackageTask
    {
    public:
        RoguelikeTask(const AsstCallback& callback, void* callback_arg);
        virtual ~RoguelikeTask() override = default;

        virtual bool set_params(const json::value& params) override;

        static constexpr const char* TaskType = "Roguelike";

    private:
        std::shared_ptr<ProcessTask> m_roguelike_task_ptr = nullptr;
        std::shared_ptr<RoguelikeRecruitTaskPlugin> m_recruit_task_ptr = nullptr;
        std::shared_ptr<RoguelikeSkillSelectionTaskPlugin> m_skill_task_ptr = nullptr;
        std::shared_ptr<RoguelikeBattleTaskPlugin> m_battle_task_ptr = nullptr;
        std::shared_ptr<RoguelikeCustomStartTaskPlugin> m_custom_start_task_ptr = nullptr;
    };
}
