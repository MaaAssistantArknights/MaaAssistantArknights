#pragma once

#include <chrono>
#include <optional>
#include <unordered_map>

#include "RoguelikeBoskyPassageMap.h"
#include "Task/Roguelike/AbstractRoguelikeTaskPlugin.h"

namespace asst
{
class RoguelikeBoskyPassageRoutingTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeBoskyPassageRoutingTaskPlugin() override = default;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual bool load_params(const json::value& params) override;
    virtual void reset_in_run_variables() override;

    enum class RoutingStrategy
    {
        None,
        BoskyPassage_JieGarden,
        FindPlaytime_JieGarden,
    };

protected:
    virtual bool _run() override;

private:
    void bosky_update_map(); // 从当前截图识别所有可见节点并更新/创建
    void bosky_decide_and_click(const std::vector<RoguelikeNodeType>& priority_order);      // 策略，可指定优先级策略
    std::vector<RoguelikeNodeType> get_bosky_passage_priority(const std::string& strategy); // 从配置文件读取优先级

    inline static std::function<std::string(RoguelikeNodeType)> type2name = &RoguelikeMapConfig::type2name;
    inline static std::function<RoguelikeNodeType(const std::string&)> name2type = &RoguelikeMapConfig::name2type;
    // ———————— constants and variables ———————————————————————————————————————————————
    RoutingStrategy m_bosky_routing_strategy = RoutingStrategy::None;
    RoguelikeBoskyPassageMap::BoskyPassageMapConfig m_bosky_config;

    // 主地图上点过树洞节点 → 下一次进图时先清空地图（每次进入树洞，布局都会重新随机）
    mutable bool m_reset_map_pending = false;
    // 本插件实例是否已经建过地图（插件随任务重建，重开任务时不会沿用上一轮残留的节点状态）
    bool m_map_initialized = false;
    // 上一次点击的节点与时间：地图很快又出现说明这次没进节点，需要撤销 visited 重试
    std::optional<size_t> m_last_clicked;
    std::chrono::steady_clock::time_point m_last_click_time {};
    std::unordered_map<size_t, int> m_click_fail_count;

    // 界园树洞地图使用单例模式，通过 RoguelikeBoskyPassageMap::get_instance() 访问
};
}
