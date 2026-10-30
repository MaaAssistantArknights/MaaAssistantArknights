#pragma once
#include <string_view>

#include "Task/Roguelike/AbstractRoguelikeTaskPlugin.h"

namespace asst
{
// 投币重铸（俗称电表倒转）：在常乐节点里反复投钱。
// 轮次编排放在 C++ 里（5 衡 1 厉、刷钱与换藏品两个阶段），单次投币的流程放在资源层，
// 客户端只负责下发次数。
class RoguelikeCoppersRecastTaskPlugin final : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    ~RoguelikeCoppersRecastTaskPlugin() override = default;

    // 读取 coppers_recast_count / coppers_recast_relic_count，仅界园且投币重铸模式启用
    bool load_params(const json::value& params) override;

    // 在常乐节点的投币入口任务上接管
    bool verify(AsstMsg msg, const json::value& details) const override;

    // 进入新的一局时清零计数
    void reset_in_run_variables() override;

protected:
    bool _run() override;

private:
    // 本轮投哪种钱
    enum class RecastType
    {
        Heng, // 衡如常
        Li,   // 厉如锋
    };

    // 本轮拿什么奖励
    enum class RecastReward
    {
        Coin,   // 收下，源石锭直接入账
        Candle, // 要酒一壶，换 6 烛火
        Relic,  // 要兵器一对，换 6 件收藏品
    };

    // 跑完一整轮：投币 → 结果对勾 → 领奖 → 收藏品弹窗 → 回面板
    bool run_once(RecastType type, RecastReward reward);

    // 等到「还能再投一次」的界面出现（钱卡面板或进节点选项）。
    // 中间如果只是剧情对话就点一下推进；既没有面板也没有事件横幅就认为已经离开这个节点。
    bool wait_for_panel();

    // 跑一个只认不点的探针任务
    bool probe(std::string_view task);

    int m_recast_count = 0; // 刷钱次数
    int m_relic_count = 0;  // 换藏品次数
    int m_finished = 0;     // 已完成的投币次数，跨常乐节点累计
};
}
