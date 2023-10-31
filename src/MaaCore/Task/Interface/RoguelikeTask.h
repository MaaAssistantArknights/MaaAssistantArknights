#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
    class ProcessTask;
    class ScreenshotTaskPlugin;
    class RoguelikeConfig;
    class RoguelikeBattleTaskPlugin;
    class RoguelikeControlTaskPlugin;
    class RoguelikeCustomStartTaskPlugin;
    class RoguelikeDebugTaskPlugin;
    class RoguelikeDifficultySelectionTaskPlugin;
    class RoguelikeFormationTaskPlugin;
    class RoguelikeLastRewardTaskPlugin;
    class RoguelikeRecruitTaskPlugin;
    class RoguelikeResetTaskPlugin;
    class RoguelikeSkillSelectionTaskPlugin;
    class RoguelikeShoppingTaskPlugin;
    class RoguelikeStageEncounterTaskPlugin;
    class RoguelikeStrategyChangeTaskPlugin;

    class RoguelikeFoldartalGainTaskPlugin;
    class RoguelikeFoldartalUseTaskPlugin;

    class RoguelikeTask : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "Roguelike";

        RoguelikeTask(const AsstCallback& callback, Assistant* inst);
        virtual ~RoguelikeTask() override = default;

        virtual bool set_params(const json::value& params) override;

    private:
        std::shared_ptr<ProcessTask> m_roguelike_task_ptr = nullptr;
        std::shared_ptr<ScreenshotTaskPlugin> m_screenshot_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeConfig> m_roguelike_config_ptr = nullptr;
        std::shared_ptr<RoguelikeControlTaskPlugin> m_control_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeRecruitTaskPlugin> m_recruit_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeSkillSelectionTaskPlugin> m_skill_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeBattleTaskPlugin> m_battle_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeCustomStartTaskPlugin> m_custom_start_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeDebugTaskPlugin> m_debug_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeDifficultySelectionTaskPlugin> m_difficulty_selection_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeFormationTaskPlugin> m_formation_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeLastRewardTaskPlugin> m_last_reward_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeResetTaskPlugin> m_reset_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeShoppingTaskPlugin> m_shopping_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeStageEncounterTaskPlugin> m_stage_encounter_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeStrategyChangeTaskPlugin> m_strategy_change_plugin_ptr = nullptr;

        std::shared_ptr<RoguelikeFoldartalGainTaskPlugin> m_foldartal_gain_plugin_ptr = nullptr;
        std::shared_ptr<RoguelikeFoldartalUseTaskPlugin> m_foldartal_use_plugin_ptr = nullptr;
    };
}
