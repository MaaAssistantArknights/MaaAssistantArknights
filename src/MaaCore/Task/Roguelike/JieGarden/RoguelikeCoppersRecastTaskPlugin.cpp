#include "RoguelikeCoppersRecastTaskPlugin.h"

#include <string>
#include <string_view>

#include "Config/TaskData.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

// 投币入口：BoskyPassage 路由把常乐节点的进入动作换成该任务后触发本插件
constexpr std::string_view StartTask = "JieGarden@Roguelike@CoppersRecast-Start";
// 地图守卫：命中即说明已经回到残识地图
constexpr std::string_view OnMapTask = "JieGarden@Roguelike@CoppersRecast-OnMap";
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
        // 投完就放弃本局并结束任务：本模式的产出在局内已经拿到，没有继续打下去的意义
        m_control_ptr->exit_then_stop(true);
        return true;
    }

    // 一个常乐节点里可以连着投很多次，所以在这里循环；一离开节点就交还给路由插件去找下一个
    bool entering_node = true;
    while (m_finished < total) {
        if (is_on_map(entering_node)) {
            // 已经站在残识地图上（这个节点的钱投完了，或者根本没点进去），剩下的交给路由
            LogInfo << __FUNCTION__ << "| on map with" << m_finished << "/" << total
                    << "done, leave the rest to routing";
            return true;
        }
        entering_node = false;

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
            return true;
        }

        ++m_finished;
    }

    LogInfo << __FUNCTION__ << "| all rounds finished:" << m_finished << "/" << total << "| exit then stop";
    m_control_ptr->exit_then_stop(true);
    return true;
}

bool asst::RoguelikeCoppersRecastTaskPlugin::is_on_map(const bool confirm_entry)
{
    if (!ProcessTask(*this, { std::string(OnMapTask) }).set_retry_times(0).run()) {
        return false;
    }
    if (!confirm_entry) {
        return true;
    }

    // 刚点进节点时界面还在过场，可能一眼看到的是地图。隔一会儿复查一次，
    // 两次都看到才认定「没进节点」，免得把这个节点空转成死循环。
    sleep(1000);
    return ProcessTask(*this, { std::string(OnMapTask) }).set_retry_times(0).run();
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
