#include "RoguelikeCoppersRecastTaskPlugin.h"

#include <string>
#include <string_view>

#include "Config/TaskData.h"
#include "Task/ProcessTask.h"
#include "Task/Roguelike/Map/RoguelikeBoskyPassageMap.h"
#include "Utils/Logger.hpp"

// 投币入口：不期而遇插件在树洞常乐节点识别到事件 ｢掷地有声｣ 后运行该任务，触发本插件
constexpr std::string_view StartTask = "JieGarden@Roguelike@CoppersRecast-Start";
// ｢还能再投一次｣ 的面板：钱卡（衡如常/厉如锋）或进节点选项（欣然应允/来就来）
constexpr std::string_view PanelTask = "JieGarden@Roguelike@CoppersRecast-Panel";
// 事件横幅：左下角挂着 ｢掷地有声｣ 说明人还在这个常乐节点里，只是停在剧情对话上
constexpr std::string_view InEventTask = "JieGarden@Roguelike@CoppersRecast-InEvent";
// 推进剧情对话（只在这种情况下才点屏幕，避免回到地图上盲点）
constexpr std::string_view TapTask = "JieGarden@Roguelike@CoppersRecast-Tap";
// 探针没命中时的占位任务（永远命中、什么都不做）。
// 探针要是直接空跑一个不存在的界面，ProcessTask 会返回失败并回调 SubTaskError，
// 客户端会当成一次任务失败（弹提示、计入「任务失败」成就），一轮探测能刷好几条。
// 把占位任务排在最后，探针永远「成功」，靠 get_last_task_name() 判断真正命中的是哪一个。
constexpr std::string_view ProbeMissTask = "JieGarden@Roguelike@CoppersRecast-ProbeMiss";
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

// 每 6 次为一轮，刷钱段和藏品段共用同一套节奏：
// 刷钱段前 5 次投「衡如常」拿 65 源石锭，藏品段前 5 次投「厉如锋」拿 6 件藏品；
// 两段的第 6 次都投「厉如锋」换 6 烛火 —— 每投一次扣 1 烛火，靠这一次补回来才投得下去。
// 投衡炸火不影响节奏；投厉炸火（没换到烛火）时 Step 会认出「来就来」原地重投一次，
// 用的还是这一轮设好的 Li + Candle，重投不占次数。
constexpr int RoundsPerCycle = 6;

// 炸火收场的终点：点掉结果页对勾就收链，插件据此知道「这一投没成」。
// 成功收在 -RoundEnd，炸火收在 -RoundFail / -LeaveDone，两者分开才能补投烛火。
constexpr std::string_view RoundFailTask = "JieGarden@Roguelike@CoppersRecast-RoundFail";
constexpr std::string_view LeaveDoneTask = "JieGarden@Roguelike@CoppersRecast-LeaveDone";
constexpr std::string_view LeaveEndTask = "JieGarden@Roguelike@CoppersRecast-LeaveEnd";
// 炸火后游戏给了重投机会才会出现的面板（「来就来」）；没有就只能点「离开」出节点
constexpr std::string_view RetryPanelTask = "JieGarden@Roguelike@CoppersRecast-RetryPanel";
// 连续补烛火最多试几次：一直炸火说明界面不对，别再烧资源
constexpr int MaxCandleRetries = 3;

// 等面板时最多看多少轮（每轮约 1.2 秒，够一段剧情对话走完）
constexpr int PanelWaitLoops = 12;

// 连续多少次「既没有面板也没有事件横幅」才认定已经离开这个节点。
// 刚点完「收下」「离开」时画面在过渡，单帧落空不代表人走了；
// 但也不能像以前那样把 12 轮全空等完（实测在地图上一动不动 35 秒），所以给个 3 次上限。
constexpr int PanelMissLoops = 3;

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
    m_need_candle = false;
    m_candle_retries = 0;
}

bool asst::RoguelikeCoppersRecastTaskPlugin::_run()
{
    LogTraceFunction;

    const int total = m_recast_count + m_relic_count;
    if (m_finished >= total) {
        LogInfo << __FUNCTION__ << "| all rounds finished:" << m_finished << "/" << total << "| exit then stop";
        // 投完只退出、不放弃：这一局的进度要留给下一次继续刷（入口链会接着打这一局）。
        // exit_then_stop 只跑退出流程，不会停掉调用方的任务链，必须手动禁用主任务
        m_control_ptr->exit_then_stop(false);
        m_task_ptr->set_enable(false);
        return true;
    }

    // 本模式的每一步都是花真资源换收益，资源里少一个任务名就会让 ProcessTask 直接报
    // UnknownTask 并回调 SubTaskError（客户端记一次任务失败，连击就弹成就），任务当场中止。
    // 这里先确认探针的占位任务在，缺了就停手并留一条明确的日志，而不是一路报错。
    if (Task.get(ProbeMissTask) == nullptr) {
        LogError << __FUNCTION__ << "| task" << ProbeMissTask
                 << "is missing in resource, stop the recast without touching the game";
        return true;
    }

    // 一个常乐节点里可以连着投很多次，所以在这里循环；面板不再出现就交还给路由插件去找下一个
    while (m_finished < total) {
        // 用户在客户端点了停止：立刻收手，别把这一轮的界面留在半路，
        // 也避免停止请求被这一层循环拖住（那会让「结束任务→再启动」卡在停止中）
        if (need_exit()) {
            LogInfo << __FUNCTION__ << "| stopped by user," << m_finished << "/" << total << "done";
            return true;
        }

        if (!wait_for_panel()) {
            // 既看不到钱卡也没有事件横幅：已经离开这个常乐节点（或者这个节点只让投一次）。
            // 离开前把这个格子重新标成「未访问」——投币是「回地图 → 再点同一个常乐节点进去」的循环，
            // 而路由插件点完就把格子记成已访问，不放开的话下一轮它会跳过常乐，
            // 去点优先级更低的邻居节点（就是「投完一次钱就乱点旁边的节点」那个现象）。
            auto& bosky_map = RoguelikeBoskyPassageMap::get_instance();
            const size_t curr = bosky_map.get_curr_pos();
            bosky_map.set_unvisited(curr);
            LogInfo << __FUNCTION__ << "| no more casts at this node," << m_finished << "/" << total
                    << "done, reopen node" << curr << "(" << bosky_map.get_node_x(curr) << ","
                    << bosky_map.get_node_y(curr) << ") for the next entry";
            return true;
        }

        // 上一投投厉炸火、没换到烛火：这一轮先补一口烛火，补完再回原节奏。
        // 补的这次不计数（重投不是计划内的那一投），成功后接着按原编号往下走。
        const bool is_retry = m_need_candle;

        RecastType type = RecastType::Li;
        RecastReward reward = RecastReward::Candle;
        bool in_coin_phase = false;
        if (!is_retry) {
            // 分两个阶段，都按「投满几次」算、跨常乐节点继续编号：
            // 刷钱段（前 recast 次）5 衡 1 厉 —— 衡如常收 65 源石锭，第 6 次厉如锋换 6 烛火；
            // 藏品段（随后 relic 次）5 厉 1 烛火 —— 厉如锋拿藏品，第 6 次仍换烛火。
            // 两段都靠每 6 次里那一次「要酒一壶」把这一轮烧掉的烛火补回来，长期才投得下去。
            in_coin_phase = m_finished < m_recast_count;
            const int index_in_phase = in_coin_phase ? m_finished : m_finished - m_recast_count;
            const bool is_candle_round = index_in_phase % RoundsPerCycle == RoundsPerCycle - 1;

            reward = is_candle_round ? RecastReward::Candle : RecastReward::Relic;
            if (in_coin_phase) {
                type = is_candle_round ? RecastType::Li : RecastType::Heng;
                reward = is_candle_round ? RecastReward::Candle : RecastReward::Coin;
            }
        }

        const char* phase = is_retry ? "candle-retry"
                                     : (in_coin_phase ? "coin(衡如常刷源石锭)" : "relic(厉如锋拿藏品)");
        LogInfo << __FUNCTION__ << "| round" << m_finished + 1 << "/" << total << "| phase:" << phase
                << "| type:" << (type == RecastType::Li ? "Li(厉如锋)" : "Heng(衡如常)")
                << "| reward:" << (reward == RecastReward::Candle ? "Candle(要酒一壶)"
                                   : reward == RecastReward::Relic ? "Relic(要兵器一对)"
                                                                   : "Coin(收下源石锭)");

        const RoundOutcome outcome = run_once(type, reward);
        // 停止请求可能正好落在这一轮中间：此时 -Round 会被打断返回「成功」，
        // 不能把它算成投完一次，也别再继续下一轮
        if (need_exit()) {
            LogInfo << __FUNCTION__ << "| stopped by user during round" << m_finished + 1;
            return true;
        }

        if (outcome == RoundOutcome::Broken) {
            // 认不出界面就不再往下走：本模式的每一步都是花真钱换收益，宁可停下任务也不盲点。
            // 不放弃本局——进度留着，玩家自己看一眼再决定怎么处理
            LogWarn << __FUNCTION__ << "| round" << m_finished + 1 << "failed, stop and keep the run";
            m_control_ptr->exit_then_stop(false);
            m_task_ptr->set_enable(false);
            return true;
        }

        if (outcome == RoundOutcome::Failed) {
            if (type == RecastType::Li) {
                // 投厉炸火：不管在刷钱段还是藏品段，都要补一次「厉如锋 + 要酒一壶」把烛火拿回来。
                // 不计数，下一轮接着补；连续几次都失败说明界面不对，停下来别再烧资源。
                if (++m_candle_retries > MaxCandleRetries) {
                    LogWarn << __FUNCTION__ << "| candle retry exceeded" << MaxCandleRetries << ", stop and keep the run";
                    m_control_ptr->exit_then_stop(false);
                    m_task_ptr->set_enable(false);
                    return true;
                }
                m_need_candle = true;
                LogInfo << __FUNCTION__ << "| li failed, cast again for candle, retry" << m_candle_retries;
            }
            else {
                // 投衡炸火不影响节奏：算作这一投已完成，继续下一轮
                LogInfo << __FUNCTION__ << "| heng failed, keep the rhythm";
                ++m_finished;
            }

            // 炸火收尾时界面只剩两种：停在「来就来」（游戏给了重投），或者停在「离开」（没给）。
            // 「来就来」留着不动，下一轮 Step 会点它把这一投重来（用的还是插件设好的卡和奖励）；
            // 停在「离开」就先点掉出节点，交给路由重新找常乐节点再进来。
            if (!probe(RetryPanelTask)) {
                LogInfo << __FUNCTION__ << "| no retry chance, leave this node";
                ProcessTask(*this, { std::string(LeaveEndTask) }).set_retry_times(0).run();
            }
            continue;
        }

        if (is_retry) {
            m_need_candle = false;
            m_candle_retries = 0;
            LogInfo << __FUNCTION__ << "| candle recovered, back to round" << m_finished + 1;
        }
        else {
            ++m_finished;
        }
    }

    LogInfo << __FUNCTION__ << "| all rounds finished:" << m_finished << "/" << total << "| exit then stop";
    m_control_ptr->exit_then_stop(false);
    m_task_ptr->set_enable(false);
    return true;
}

bool asst::RoguelikeCoppersRecastTaskPlugin::probe(const std::string_view task)
{
    // 末尾挂一个永远命中的占位任务：探针落空时流程走占位、返回成功，
    // 就不会有 SubTaskError 冒到客户端去（那会被当成任务失败）。
    ProcessTask probe_task(*this, { std::string(task), std::string(ProbeMissTask) });
    probe_task.set_retry_times(0);
    if (!probe_task.run()) {
        return false;
    }
    return std::string_view(probe_task.get_last_task_name()) == task;
}

bool asst::RoguelikeCoppersRecastTaskPlugin::wait_for_panel()
{
    int miss_loops = 0;
    for (int i = 0; i < PanelWaitLoops; ++i) {
        if (need_exit()) {
            return false;
        }
        // 一次 ProcessTask 同时认「钱卡面板」和「事件横幅」：
        // 以前分两次跑（每次都要重新截图 + 跑两条识别），一轮空等能白花三四秒。
        ProcessTask probe_task(
            *this, { std::string(PanelTask), std::string(InEventTask), std::string(ProbeMissTask) });
        probe_task.set_retry_times(0);
        if (!probe_task.run()) {
            return false;
        }
        const std::string_view last = probe_task.get_last_task_name();
        if (last == PanelTask) {
            return true;
        }
        // 还在事件里就点一下推进对话
        if (last == InEventTask) {
            miss_loops = 0;
            ProcessTask(*this, { std::string(TapTask) }).set_retry_times(0).run();
            continue;
        }
        // 横幅和面板都不在了：可能是过渡帧，也可能是已经回到地图。
        // 连续几次都看不到才认输，交还给路由插件去找下一个常乐节点。
        if (++miss_loops >= PanelMissLoops) {
            LogInfo << __FUNCTION__ << "| panel and event banner both gone for" << miss_loops
                    << "loops, back to map";
            return false;
        }
        if (!sleep(400)) {
            return false;
        }
    }
    return false;
}

asst::RoguelikeCoppersRecastTaskPlugin::RoundOutcome
    asst::RoguelikeCoppersRecastTaskPlugin::run_once(const RecastType type, const RecastReward reward)
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

    ProcessTask round(*this, { std::string(RoundTask) });
    round.set_retry_times(0);
    const bool ok = round.run();
    const std::string last = round.get_last_task_name();

    // 看这一轮收在哪个终点：炸火收在 -RoundFail（点掉对勾就收链）或 -LeaveDone（点完离开收链），
    // 成功才收在 -RoundEnd。以前两个都收在 -RoundEnd，插件分不出成败，也就没法补投烛火。
    if (last == RoundFailTask || last == LeaveDoneTask) {
        return RoundOutcome::Failed;
    }
    if (!ok) {
        return RoundOutcome::Broken;
    }
    return RoundOutcome::Ok;
}
