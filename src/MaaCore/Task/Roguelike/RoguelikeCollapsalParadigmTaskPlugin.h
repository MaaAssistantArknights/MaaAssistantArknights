#pragma once
#include "AbstractRoguelikeTaskPlugin.h"
#include "Common/AsstTypes.h"

namespace asst
{
    class RoguelikeCollapsalParadigmTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        virtual ~RoguelikeCollapsalParadigmTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

        bool check_collapsal_paradigm_banner();

    protected:
        virtual bool _run() override;

    private:
        const Rect m_roi = { 655, 20, 10, 10 };  // 屏幕上方居中区域，点击以检查坍缩状态
        const Point m_swipe_begin = {640, 300}; // 滑动起点
        const Point m_swipe_end = {640, 0};     // 滑动终点

        mutable bool m_check_banner = false;
        mutable bool m_check_panel = false;
        mutable bool m_verification_check = false;
        mutable std::string m_zone;

        const std::unordered_set<std::string_view> m_banner_triggers_start = {
            "Sami@Roguelike@MissionCompletedFlag"
            // "Sami@Roguelike@DropsFlag"
        };

        const std::unordered_set<std::string_view> m_banner_triggers_completed = {
            "Sami@Roguelike@GetDrop1",
            "Sami@Roguelike@GetDrop2",
            "Sami@Roguelike@GetDrop3",
            "Sami@Roguelike@GetDrop4",
            "Sami@Roguelike@GetDropSelectReward",
            "Sami@Roguelike@GetDropSelectReward2",
            // "Sami@Roguelike@ClickToDrops",
            "Sami@Roguelike@TraderRandomShoppingConfirm"
        };

        const std::unordered_set<std::string_view> m_panel_triggers = {
            // 作战
            "Sami@Roguelike@StageCombatDps",
            "Sami@Roguelike@StageCombatDpsAI6",
            "Sami@Roguelike@StageVerticalCombatDps",
            "Sami@Roguelike@StageVerticalCombatDpsAI6",
            // 紧急作战
            "Sami@Roguelike@StageEmergencyDps",
            "Sami@Roguelike@StageEmergencyDpsAI6",
            "Sami@Roguelike@StageVerticalEmergencyDps",
            "Sami@Roguelike@StageVerticalEmergencyDpsAI6",
            // 险路恶敌
            "Sami@Roguelike@StageDreadfulFoe",
            "Sami@Roguelike@StageDreadfulFoe-5",
            // 不期而遇
            "Sami@Roguelike@StageEncounter",
            "Sami@Roguelike@StageEncounterAI6",
            "Sami@Roguelike@StageVerticalEncounter",
            "Sami@Roguelike@StageVerticalEncounterAI6",
            // 安全的角落
            "Sami@Roguelike@StageSafeHouse",
            "Sami@Roguelike@StageSafeHouseAI6",
            "Sami@Roguelike@StageVerticalSafeHouse",
            "Sami@Roguelike@StageVerticalSafeHouseAI6",
            // 兴致盎然
            "Sami@Roguelike@StageGambling",
            "Sami@Roguelike@StageGamblingAI6",
            "Sami@Roguelike@StageVerticalGambling",
            "Sami@Roguelike@StageVerticalGamblingAI6",
            // 命运所指
            "Sami@Roguelike@StageProphecy",
            "Sami@Roguelike@StageProphecyAI6",
            "Sami@Roguelike@StageVerticalProphecy",
            "Sami@Roguelike@StageVerticalProphecyAI6",
            // 得偿所愿
            "Sami@Roguelike@StageBoons",
            "Sami@Roguelike@StageBoonsAI6",
            "Sami@Roguelike@StageVerticalBoons",
            "Sami@Roguelike@StageVerticalBoonsAI6",
            // 诡意行商
            "Sami@Roguelike@StageTrader",
            "Sami@Roguelike@StageTraderAI6",
            "Sami@Roguelike@StageVerticalTrader",
            "Sami@Roguelike@StageVerticalTraderAI6",
            // 失与得
            "Sami@Roguelike@StageWindAndRain",
            "Sami@Roguelike@StageWindAndRainAI6",
            "Sami@Roguelike@StageVerticalWindAndRain",
            "Sami@Roguelike@StageVerticalWindAndRainAI6",
            // 先行一步
            "Sami@Roguelike@StageEmergencyTransportation",
            "Sami@Roguelike@StageEmergencyTransportationAI6",
            "Sami@Roguelike@StageVerticalEmergencyTransportation",
            "Sami@Roguelike@StageVerticalEmergencyTransportationAI6",
            // 树篱之途
            "Sami@Roguelike@StageBoskyPassage",
            "Sami@Roguelike@StageBoskyPassageAI6",
            "Sami@Roguelike@StageVerticalBoskyPassage",
            "Sami@Roguelike@StageVerticalBoskyPassageAI6",
            // 模糊的预感
            "Sami@Roguelike@StageMysteriousPresage",
            "Sami@Roguelike@StageFerociousPresage"
        };

        const std::unordered_map<std::string_view, std::string> m_zone_dict = {
            {"Sami@Roguelike@OnStage_0", "深埋迷境"},
            {"Sami@Roguelike@OnStage_1", "初霜湖泽"},
            {"Sami@Roguelike@OnStage_2", "密静林地"},
            {"Sami@Roguelike@OnStage_3", "昧明冻土"},
            {"Sami@Roguelike@OnStage_4", "积冰岩骸"},
            {"Sami@Roguelike@OnStage_5", "无瑕花园"},
            {"Sami@Roguelike@OnStage_6", "远见之构"},
            {"Sami@Roguelike@OnStage_7", "永恒之尘"}
        };

        bool new_zone() const;

        bool check_collapsal_paradigm_panel();

        void toggle_collapsal_status_panel();
        void exit_then_restart();
        void exit_then_stop();
        void wait_for_loading(unsigned int millisecond = 0);
        void wait_for_stage(unsigned int millisecond = 0);
        void clp_pd_callback(std::string cur, int deepen_or_weaken = 0, std::string prev = "");
    };
}
