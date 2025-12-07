#include "RoguelikeCoppersRecastPlugin.h"

#include <algorithm>
#include <array>
#include <string_view>

#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Roguelike/RoguelikeParameterAnalyzer.h"

namespace
{
std::optional<int> resolve_metric(
    asst::RoguelikeCoppersRecastPlugin::Condition::ConditionType type,
    const asst::RoguelikeCoppersRecastPlugin::Snapshot& snapshot)
{
    using ConditionType = asst::RoguelikeCoppersRecastPlugin::Condition::ConditionType;
    switch (type) {
    case ConditionType::Count:
        return snapshot.recast_times;
    case ConditionType::HP:
        return snapshot.hp;
    case ConditionType::Hope:
        return snapshot.hope;
    case ConditionType::Ingot:
        return snapshot.ingot;
    case ConditionType::Ticket:
        return snapshot.ticket_count;

    default:
        return std::nullopt;
    }
}
}

bool asst::RoguelikeCoppersRecastPlugin::Condition::evaluate(const Snapshot& snapshot) const
{
    const auto target_value_opt = resolve_metric(condition_type, snapshot);
    if (!target_value_opt) {
        return false;
    }

    const int target_value = *target_value_opt;
    switch (compare_type) {
    case CompareType::EqualTo:
        return target_value == value;
    case CompareType::GreaterThan:
        return target_value > value;
    case CompareType::LessThan:
        return target_value < value;
    case CompareType::GreaterEqual:
        return target_value >= value;
    case CompareType::LessEqual:
        return target_value <= value;
    default:
        return false;
    }
}

namespace
{
using ConditionType = asst::RoguelikeCoppersRecastPlugin::Condition::ConditionType;
using CompareType = asst::RoguelikeCoppersRecastPlugin::Condition::CompareType;

std::optional<ConditionType> parse_condition_type(std::string_view metric)
{
    static constexpr std::array<std::pair<std::string_view, ConditionType>, 5> Mapping = {
        { { "recast_count", ConditionType::Count },
          { "hp", ConditionType::HP },
          { "hope", ConditionType::Hope },
          { "ingot", ConditionType::Ingot },
          { "ticket", ConditionType::Ticket } }
    };

    const auto iter =
        std::find_if(Mapping.cbegin(), Mapping.cend(), [&](const auto& item) { return item.first == metric; });
    if (iter == Mapping.cend()) {
        return std::nullopt;
    }
    return iter->second;
}

std::optional<CompareType> parse_compare_type(std::string_view comparator)
{
    static constexpr std::array<std::pair<std::string_view, CompareType>, 5> Mapping = {
        { { "less", CompareType::LessThan },
          { "less_or_equal", CompareType::LessEqual },
          { "equal", CompareType::EqualTo },
          { "greater", CompareType::GreaterThan },
          { "greater_or_equal", CompareType::GreaterEqual } }
    };

    const auto iter =
        std::find_if(Mapping.cbegin(), Mapping.cend(), [&](const auto& item) { return item.first == comparator; });
    if (iter == Mapping.cend()) {
        return std::nullopt;
    }
    return iter->second;
}
}

bool asst::RoguelikeCoppersRecastPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view == "MiniGame@CoppersRecast@Start") {
        return true;
    }

    return false;
}

bool asst::RoguelikeCoppersRecastPlugin::load_params(const json::value& params)
{
    std::vector<Condition> conditions;

    if (auto array_opt = params.find<json::array>("conditions")) {
        conditions.reserve(array_opt->size());
        for (const auto& cond_value : *array_opt) {
            if (!cond_value.is_object()) {
                continue;
            }
            const auto& cond_obj = cond_value.as_object();
            auto metric = parse_condition_type(cond_obj.get("metric", std::string()));
            auto comparator = parse_compare_type(cond_obj.get("comparator", std::string()));
            if (!metric || !comparator) {
                Log.warn("RoguelikeCoppersRecastPlugin: skip invalid condition", cond_value.to_string());
                continue;
            }
            conditions.emplace_back(cond_obj.get("threshold", 0), *metric, *comparator);
        }
    }

    set_conditions(std::move(conditions));
    if (m_conditions.empty()) {
        Log.warn("RoguelikeCoppersRecastPlugin: no valid conditions configured; recast will not auto-stop");
    }

    return true;
}

void asst::RoguelikeCoppersRecastPlugin::set_conditions(std::vector<Condition> conditions) noexcept
{
    m_conditions = std::move(conditions);
}

void asst::RoguelikeCoppersRecastPlugin::clear_conditions() noexcept
{
    m_conditions.clear();
}

void asst::RoguelikeCoppersRecastPlugin::reset_snapshot() noexcept
{
    m_snapshot = {};
}

bool asst::RoguelikeCoppersRecastPlugin::_run()
{
    LogTraceFunction;

    reset_snapshot();

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
            return true;
        }

        if (!ProcessTask(*this, { "JieGarden@Roguelike@CoppersRecast" }).run()) {
            Log.error(__FUNCTION__, "| RoguelikeCoppersRecastPlugin: recast task failed, stopping recast");
            return false;
        }

        ++m_snapshot.recast_times;
    }

    return false;
}
