#include "RoguelikeCoppersRecastTaskPlugin.h"

#include <string>
#include <string_view>

#include "Config/TaskData.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

// 投币入口：不期而遇插件在树洞常乐节点识别到事件 ｢掷地有声｣ 后运行该任务，触发本插件
constexpr std::string_view StartTask = "JieGarden@Roguelike@CoppersRecast-Start";
// ｢还能再投一次｣ 的面板：钱卡（衡如常/厉如锋）或进节点选项（欣然应允/来就来）
constexpr std::string_view PanelTask = "JieGarden@Roguelike@CoppersRecast-Panel";
// 事件横幅：左下角挂着 ｢掷地有声｣ 说明人还在这个常乐节点里，只是停在剧情对话上
constexpr std::string_view InEventTask = "JieGarden@Roguelike@CoppersRecast-InEvent";
// 推进剧情对话（只在这种情况下才点屏幕，避免回到地图上盲点）
constexpr std::string_view TapTask = "JieGarden@Roguelike@CoppersRecast-Tap";
// 一整轮的投币流程
constexpr std::string_view RoundTask = "JieGarden@Roguelike@CoppersRecast-Round";
// 投哪种钱、拿什么奖励，由插件在开跑前重设基准任务
constexpr std::string_view PickTask = "JieGarden@Roguelike@CoppersRecast-Pick";
constexpr std::string_view PickHengTask = "JieGarden@Roguelike@CoppersRecast-PickHeng";
constexpr std::string_view PickLiTask = "JieGarden@Roguelike@CoppersRecast-PickLi";
constexpr std::string_view RewardTask = "JieGarden@Roguelike@CoppersRecast-Reward";
constexpr std::string_view RewardCoinTask = "JieGarden@Roguelike@CoppersRecast-RewardCoin";
constexpr std::string_view RewardCandleTask = "JieGarden@Roguelike@CoppersRecast-RewardCandle";
constexpr std::string_view RewardRelicTask = "JieGarden@Roguelike@CoppersRecast-RewardRelic";

// 每 6 次为一轮：前 5 次投衡如常刷源石锭 / 投厉如锋拿收藏品，第 6 次投厉如锋换烛火。
// 厉如锋要烧烛火，靠第 6 次换回来的烛火补上，长期挂机烛火才能持平。
constexpr int RoundsPerCycle = 6;

// 等面板时最多看多少轮（每轮约 1.2 秒，够一段剧情对话走完）
constexpr int PanelWaitLoops = 12;

bool asst::RoguelikeCoppersRecastTaskPlugin::load_params([[maybe_unused]] const json::value& params)
{
    if (m_config->get_theme() != RoguelikeTheme::JieGarden) {
        return false;
    }
    if (m_config->get_mode() != RoguelikeMode::CoppersRecast) {
        return false;
    }

    m_recast_count = m_config->get_coppers_recast_count();
    m_relic_count = m_config->get_coppers_recast_relic_count();
    m_finished = 0;

    LogInfo << __FUNCTION__ << "| recast:" << m_recast_count << "| relic:" << m_relic_count;
    return m_recast_count + m_relic_count > 0;
}

bool asst::RoguelikeCoppersRecastTaskPlugin::verify(const AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    return details.get("details", "task", "").ends_with(StartTask);
}

void asst::RoguelikeCoppersRecastTaskPlugin::reset_in_run_variables()
{
    m_finished = 0;
}

bool asst::RoguelikeCoppersRecastTaskPlugin::_run()
{
    LogTraceFunction;

    const int total = m_recast_count + m_relic_count;
    if (m_finished >= total) {
        LogInfo << __FUNCTION__ << "| all rounds finished:" << m_finished << "/" << total << "| exit then stop";
        // 投完就放弃本局并结束任务：本模式的产出在局内已经拿到，没有继续打下去的意义。
        // exit_then_stop 只跑放弃流程，不会停掉调用方的任务链，必须手动禁用主任务
        m_control_ptr->exit_then_stop(true);
        m_task_ptr->set_enable(false);
        return true;
    }

    // 一个常乐节点里可以连着投很多次，所以在这里循环；面板不再出现就交还给路由插件去找下一个
    while (m_finished < total) {
        if (!wait_for_panel()) {
            // 既看不到钱卡也没有事件横幅：已经离开这个常乐节点（或者这个节点只让投一次）
            LogInfo << __FUNCTION__ << "| no more casts at this node," << m_finished << "/" << total
                    << "done, leave the rest to routing";
            return true;
        }

        // 先跑完刷钱阶段，再进入换藏品阶段；两个阶段各自按 6 次一轮重新编号
        const bool in_coin_phase = m_finished < m_recast_count;
        const int index_in_phase = in_coin_phase ? m_finished : m_finished - m_recast_count;
        const bool is_candle_round = index_in_phase % RoundsPerCycle == RoundsPerCycle - 1;

        RecastType type = RecastType::Li;
        RecastReward reward = is_candle_round ? RecastReward::Candle : RecastReward::Relic;
        if (in_coin_phase) {
            type = is_candle_round ? RecastType::Li : RecastType::Heng;
            reward = is_candle_round ? RecastReward::Candle : RecastReward::Coin;
        }

        LogInfo << __FUNCTION__ << "| round" << m_finished + 1 << "/" << total << "| coin phase:" << in_coin_phase
                << "| type:" << (type == RecastType::Li ? "Li" : "Heng") << "| reward:" << static_cast<int>(reward);

        if (!run_once(type, reward)) {
            // 认不出界面就不再往下走：本模式的每一步都是花真钱换收益，宁可放弃本局重开也不盲点
            LogWarn << __FUNCTION__ << "| round" << m_finished + 1 << "failed, abandon this run";
            m_control_ptr->exit_then_stop(true);
            m_task_ptr->set_enable(false);
            return true;
        }

        ++m_finished;
    }

    LogInfo << __FUNCTION__ << "| all rounds finished:" << m_finished << "/" << total << "| exit then stop";
    m_control_ptr->exit_then_stop(true);
    m_task_ptr->set_enable(false);
    return true;
}

bool asst::RoguelikeCoppersRecastTaskPlugin::probe(const std::string_view task)
{
    return ProcessTask(*this, { std::string(task) }).set_retry_times(0).run();
}

bool asst::RoguelikeCoppersRecastTaskPlugin::wait_for_panel()
{
    for (int i = 0; i < PanelWaitLoops; ++i) {
        if (probe(PanelTask)) {
            return true;
        }
        // 还在事件里就点一下推进对话；横幅都不在了说明已经回到地图，不再点任何东西
        if (probe(InEventTask)) {
            ProcessTask(*this, { std::string(TapTask) }).set_retry_times(0).run();
            continue;
        }
        sleep(600);
    }
    return false;
}

bool asst::RoguelikeCoppersRecastTaskPlugin::run_once(const RecastType type, const RecastReward reward)
{
    Task.set_task_base(std::string(PickTask), std::string(type == RecastType::Li ? PickLiTask : PickHengTask));

    std::string_view reward_task = RewardCoinTask;
    switch (reward) {
    case RecastReward::Coin:
        reward_task = RewardCoinTask;
        break;
    case RecastReward::Candle:
        reward_task = RewardCandleTask;
        break;
    case RecastReward::Relic:
        reward_task = RewardRelicTask;
        break;
    }
    Task.set_task_base(std::string(RewardTask), std::string(reward_task));

    return ProcessTask(*this, { std::string(RoundTask) }).set_retry_times(0).run();
}
