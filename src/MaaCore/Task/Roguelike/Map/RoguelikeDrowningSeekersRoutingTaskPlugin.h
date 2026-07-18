#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "MaaUtils/NoWarningCVMat.hpp"
#include "RoguelikeDrowningSeekersMap.h"
#include "Task/Roguelike/AbstractRoguelikeTaskPlugin.h"
#include "Vision/Roguelike/RoguelikeDrowningSeekersMapAnalyzer.h"

namespace asst
{
// ———————— 策略抽象 ————————
// 迷宫导航一步的决策：放弃本局，或移动到与玩家相邻的某个格。
struct DrowningSeekersRouteDecision
{
    enum class Action
    {
        Abandon, // 放弃本局（ExitThenAbandon）
        Move,    // 点击 next（玩家相邻格），朝 target 前进
    };

    Action action = Action::Abandon;
    std::pair<int, int> next = { -1, -1 };   // 本步要点击的格（玩家相邻）
    std::pair<int, int> target = { -1, -1 }; // 最终目标格（用于日志）
    std::string reason;                      // 决策原因（日志/回调用）
};

class IDrowningSeekersRoutingStrategy
{
public:
    virtual ~IDrowningSeekersRoutingStrategy() = default;
    virtual DrowningSeekersRouteDecision decide(const RoguelikeDrowningSeekersMap& map, int action_points) const = 0;
};

// 投资模式策略：
//   1. 行动力未知或 <= abandon_ap → 放弃；
//   2. 有未 ✓ 的诡意行商 → 走向最近者（软避战：进入作战/紧急作战/未知的凶戾格罚 +1000）；
//   3. 否则走「未知的诡秘」最多的路线（并列取代价小者）；
//   4. 图中无行商也无诡秘 → 放弃。
class DrowningSeekersInvestmentStrategy final : public IDrowningSeekersRoutingStrategy
{
public:
    explicit DrowningSeekersInvestmentStrategy(int abandon_ap) :
        m_abandon_ap(abandon_ap)
    {
    }

    virtual DrowningSeekersRouteDecision
        decide(const RoguelikeDrowningSeekersMap& map, int action_points) const override;

private:
    int m_abandon_ap = 1;
};

// 黑流树海（DrowningSeekers）迷宫地图导航插件。
//
// 流程：确保地图缩小 → 滑动固定视野 → 截图交 Analyzer 识别（ONNX）→ OCR 行动力
//       → 策略决策（放弃 / 移动到相邻格）→ set_task_base 衔接后续节点流程。
// 每次回到地图界面都重新识别、重新规划（不跨步持久化）。
class RoguelikeDrowningSeekersRoutingTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeDrowningSeekersRoutingTaskPlugin() override = default;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual bool load_params(const json::value& params) override;
    virtual void reset_in_run_variables() override;

protected:
    virtual bool _run() override;

private:
    // 从当前截图识别地图（缩放 + 滑动 + Analyzer），失败返回 valid=false 的结果。
    RoguelikeDrowningSeekersMapAnalyzer::Result recognize_map();

    // 由 Analyzer 结果构建 RoguelikeDrowningSeekersMap。
    static RoguelikeDrowningSeekersMap build_map(const RoguelikeDrowningSeekersMapAnalyzer::Result& result);

    // OCR 右上角行动力（剩余步数）。失败返回 -1。
    int recognize_action_points();

    // 把识别结果详细打印到日志，便于人工核对。
    void dump_recognition(const RoguelikeDrowningSeekersMapAnalyzer::Result& result, int action_points) const;

    // 决策为放弃时的收尾
    void act_abandon(const std::string& reason);
    // 决策为移动时的收尾（点击 + 衔接节点进入流程）
    void act_move(const RoguelikeDrowningSeekersMap& map, const DrowningSeekersRouteDecision& decision);

    inline static std::function<std::string(RoguelikeNodeType)> type2name = &RoguelikeMapConfig::type2name;

    // ———————— 常量与变量 ————————
    std::unique_ptr<IDrowningSeekersRoutingStrategy> m_strategy;
    int m_grid_step = 101;       // specialParams[0]
    int m_nameplate_offset = 16; // specialParams[1]
    int m_abandon_ap = 1;        // specialParams[2]，AP<=此值放弃
    bool m_dry_run = true;       // specialParams[3]，1=仅识别安全退出（人工核对识别用）
};
}
