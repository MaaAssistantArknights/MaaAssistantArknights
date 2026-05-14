/**
 * @file RoguelikeCoppersRecastPlugin.cpp
 * @brief Implementation of the JieGarden roguelike coppers recast plugin
 *
 * This plugin automates the coppers recast mini-game in JieGarden roguelike mode,
 * continuously recasting until user-defined conditions are met.
 *
 * 界园肉鸽通宝重投插件的实现
 * 该插件自动化界园肉鸽模式中的通宝重投小游戏,持续重投直到满足用户定义的条件
 */

#include "RoguelikeCoppersRecastPlugin.h"

#include <algorithm>

#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Roguelike/RoguelikeParameterAnalyzer.h"

bool asst::RoguelikeCoppersRecastPlugin::Condition::evaluate(const Snapshot& snapshot) const
{
    if (condition_type == ConditionType::None) {
        Log.warn(__FUNCTION__, "| Condition has invalid ConditionType::None, will never evaluate to true");
        return false;
    }

    const auto target_value_opt = [&]() -> std::optional<int> {
        switch (condition_type) {
        case ConditionType::recast_count:
            return snapshot.recast_times;
        case ConditionType::hp:
            return snapshot.hp;
        case ConditionType::hope:
            return snapshot.hope;
        case ConditionType::ingot:
            return snapshot.ingot;
        case ConditionType::ticket:
            return snapshot.ticket_count;
        default:
            return std::nullopt;
        }
    }();

    if (!target_value_opt) {
        return false;
    }

    const int target_value = *target_value_opt;
    switch (compare_type) {
    case CompareType::equal:
        return target_value == value;
    case CompareType::greater:
        return target_value > value;
    case CompareType::less:
        return target_value < value;
    case CompareType::greater_or_equal:
        return target_value >= value;
    case CompareType::less_or_equal:
        return target_value <= value;
    default:
        return false;
    }
}

bool asst::RoguelikeCoppersRecastPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart) {
        return false;
    }

    auto subtask_opt = details.find<std::string>("subtask");
    if (!subtask_opt || *subtask_opt != "ProcessTask") {
        return false;
    }

    std::string task = details.get("details", "task", std::string());
    return task == "MiniGame@CoppersRecast@Start";
}

bool asst::RoguelikeCoppersRecastPlugin::load_params(const json::value& params)
{
    auto conditions_opt = params.find<json::array>("conditions");
    if (!conditions_opt) {
        // Treat missing conditions as a valid configuration meaning "run indefinitely".
        // CustomTask::set_params calls load_params unconditionally for all custom tasks,
        // so this is expected for unrelated MiniGame invocations that don't pass conditions.
        m_conditions.clear();
        Log.info(__FUNCTION__, "| conditions key not found in params, using empty conditions (run indefinitely)");
        return true;
    }

    m_conditions = conditions_opt->as<std::vector<Condition>>();

    if (m_conditions.empty()) {
        Log.info(__FUNCTION__, "| No conditions specified, recast will run indefinitely");
    }
    else {
        Log.info(__FUNCTION__, "| Successfully loaded", m_conditions.size(), "condition(s)");
    }

    return true;
}

bool asst::RoguelikeCoppersRecastPlugin::_run()
{
    LogTraceFunction;

    m_snapshot = {};

    ProcessTask open_task(*this, { "JieGarden@Roguelike@CoppersOpenBox" });
    open_task.set_retry_times(0);
    open_task.set_ignore_error(true);
    open_task.run();

    while (true) {
        if (need_exit()) {
            Log.info(__FUNCTION__, "| RoguelikeCoppersRecastPlugin: stop requested by user");
            return true;
        }

        const auto image = ctrler()->get_image();

        const int hp_val = RoguelikeParameterAnalyzer::update_hp(image);
        m_snapshot.hp = hp_val >= 0 ? std::optional<int>(hp_val) : std::nullopt;

        const int hope_val = RoguelikeParameterAnalyzer::update_hope(image, "JieGarden");
        m_snapshot.hope = hope_val >= 0 ? std::optional<int>(hope_val) : std::nullopt;

        const int ingot_val = RoguelikeParameterAnalyzer::update_ingot(image, "JieGarden");
        m_snapshot.ingot = ingot_val >= 0 ? std::optional<int>(ingot_val) : std::nullopt;

        const int ticket_val = RoguelikeParameterAnalyzer::update_ticket_count(image, "JieGarden");
        m_snapshot.ticket_count = ticket_val >= 0 ? std::optional<int>(ticket_val) : std::nullopt;

        // 判断是否满足停止条件
        const bool should_stop = std::any_of(m_conditions.cbegin(), m_conditions.cend(), [&](const auto& cond) {
            return cond.evaluate(m_snapshot);
        });

        Log.debug(
            __FUNCTION__,
            std::format(
                "| RoguelikeCoppersRecastPlugin: snapshot: times={}, hp={}, hope={}, ingot={}, ticket={}",
                m_snapshot.recast_times,
                m_snapshot.hp ? std::to_string(*m_snapshot.hp) : "N/A",
                m_snapshot.hope ? std::to_string(*m_snapshot.hope) : "N/A",
                m_snapshot.ingot ? std::to_string(*m_snapshot.ingot) : "N/A",
                m_snapshot.ticket_count ? std::to_string(*m_snapshot.ticket_count) : "N/A"));

        if (should_stop) {
            Log.info(__FUNCTION__, "| RoguelikeCoppersRecastPlugin: condition met, stopping recast");
            auto stop_info = basic_info_with_what("CoppersRecastStopped");
            stop_info["details"]["recast_times"] = m_snapshot.recast_times;
            stop_info["details"]["hp"] = m_snapshot.hp.value_or(-1);
            stop_info["details"]["hope"] = m_snapshot.hope.value_or(-1);
            stop_info["details"]["ingot"] = m_snapshot.ingot.value_or(-1);
            stop_info["details"]["ticket_count"] = m_snapshot.ticket_count.value_or(-1);
            callback(AsstMsg::SubTaskExtraInfo, stop_info);
            return true;
        }

        auto progress_info = basic_info_with_what("CoppersRecastProgress");
        progress_info["details"]["recast_times"] = m_snapshot.recast_times;
        progress_info["details"]["hp"] = m_snapshot.hp.value_or(-1);
        progress_info["details"]["hope"] = m_snapshot.hope.value_or(-1);
        progress_info["details"]["ingot"] = m_snapshot.ingot.value_or(-1);
        progress_info["details"]["ticket_count"] = m_snapshot.ticket_count.value_or(-1);
        callback(AsstMsg::SubTaskExtraInfo, progress_info);

        if (!ProcessTask(*this, { "JieGarden@Roguelike@CoppersRecast" }).run()) {
            Log.error(__FUNCTION__, "| RoguelikeCoppersRecastPlugin: recast task failed, stopping recast");
            auto stop_info = basic_info_with_what("CoppersRecastStopped");
            stop_info["details"]["recast_times"] = m_snapshot.recast_times;
            stop_info["details"]["hp"] = m_snapshot.hp.value_or(-1);
            stop_info["details"]["hope"] = m_snapshot.hope.value_or(-1);
            stop_info["details"]["ingot"] = m_snapshot.ingot.value_or(-1);
            stop_info["details"]["ticket_count"] = m_snapshot.ticket_count.value_or(-1);
            callback(AsstMsg::SubTaskExtraInfo, stop_info);
            return true;
        }

        ++m_snapshot.recast_times;
    }
}
