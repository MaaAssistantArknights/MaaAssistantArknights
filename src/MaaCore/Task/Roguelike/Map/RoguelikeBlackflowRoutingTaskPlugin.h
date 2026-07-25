#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "Config/Roguelike/RoguelikeBlackflowRoutingConfig.h"
#include "RoguelikeBlackflowRoutePlanner.h"
#include "Task/Roguelike/AbstractRoguelikeTaskPlugin.h"
#include "Vision/Roguelike/RoguelikeBlackflowMapAnalyzer.h"

namespace asst
{
// 黑流树海（Blackflow）迷宫地图导航插件。
// 每回合重新识别地图、行动力和加工品面板，规划器只执行首个动作。
// 节点类型仍由 RoguelikeBlackflowMapAnalyzer 的当前实现提供。
class RoguelikeBlackflowRoutingTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeBlackflowRoutingTaskPlugin() override = default;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual bool load_params(const json::value& params) override;
    virtual void reset_in_run_variables() override;

protected:
    virtual bool _run() override;

private:
    RoguelikeBlackflowMapAnalyzer::Result recognize_map();
    int recognize_action_points(const char* task_name);

    struct GearCardHit
    {
        std::string name;
        int uses = -1;
        bool loaded = false;
        int name_cy = 0;
    };

    struct GearPanelInfo
    {
        bool valid = false;
        std::vector<std::pair<std::string, int>> uses_by_name;
        std::string loaded_name;
    };

    void ensure_gear_panel_closed();
    void open_gear_panel();
    std::vector<GearCardHit> ocr_gear_cards();
    GearPanelInfo read_gear_panel();
    bool select_gear_card(const std::string& name);

    blackflow::PlannerMap build_planner_map(
        const RoguelikeBlackflowMapAnalyzer::Result& result) const;
    std::vector<blackflow::PlannerGear>
        build_planner_gears(const GearPanelInfo& panel, std::vector<std::string>& gear_names) const;

    void dump_recognition(const RoguelikeBlackflowMapAnalyzer::Result& result, int action_points) const;
    void act_abandon(const std::string& reason);
    void act_retry(const std::string& reason);

    inline static std::function<std::string(RoguelikeNodeType)> type2name = &RoguelikeMapConfig::type2name;

    const BlackflowStrategyProfile* m_profile = nullptr;
    int m_grid_step = 101;
    int m_nameplate_offset = 16;
    int m_abandon_ap = 1;
    bool m_dry_run = true;
    int m_consecutive_failures = 0;
    bool m_force_zoom_reset_after_layer_transition = false;
    bool m_blackflow_no_boss = false;
    bool m_no_boss_floor_three = false;
    int m_no_boss_target_encounter = -1;
};
}
