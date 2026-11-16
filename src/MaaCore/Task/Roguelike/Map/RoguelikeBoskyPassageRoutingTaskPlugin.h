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

    // 常乐节点在地图上只有一种图标，分不出是令/黍/年。逐个点开候选节点，
    // 从右侧详情面板的「可以押上源石锭和令掷币取乐」里读出代理人，记到节点上。
    // 详情面板会盖住右边几列节点，所以认出「不是令」之后要点一下空白处把它收起来再点下一个。
    // 一旦认出是令就停手：面板正停在它身上，直接走进入流程，不用再点一次。
    void identify_playtime_nodes(const std::vector<size_t>& nodes);
    // 读当前屏幕上的详情面板，返回识别出的代理人；认不出返回 Unknown
    RoguelikeBoskySubNodeType probe_playtime_subtype();
    // 点地图空白处收起详情面板，并确认真的收起来了
    void close_playtime_detail();

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
    // 上一轮地图扫描有没有扫到常乐节点：扫到过就说明这张图上确实有常乐，
    // 下一轮可以先用「只认常乐」的单模板快扫（0.6 秒），扫不到再退回全量（7 秒）。
    bool m_playtime_seen_last_scan = false;
    // 最后一次「为了看详情」而点开的常乐节点：它的详情面板还开着，
    // 如果最后选中的就是它，不用再点一次（重复点同一节点会取消选中，反而没得进）
    std::optional<size_t> m_last_probed_playtime;

    // 界园树洞地图使用单例模式，通过 RoguelikeBoskyPassageMap::get_instance() 访问
};
}
