#include "RoguelikeIterateMonthlySquadPlugin.h"

#include "Config/Roguelike/RoguelikeMonthlySquadConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/OCRer.h"

bool asst::RoguelikeIterateMonthlySquadPlugin::load_params([[maybe_unused]] const json::value& params)
{
    m_checkComms = params.find<bool>("monthly_squad_check_comms").value_or(false);

    m_iterateMS = params.find<bool>("monthly_squad_auto_iterate").value_or(false);
    return m_config->get_mode() == RoguelikeMode::Squad;
}

bool asst::RoguelikeIterateMonthlySquadPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (!RoguelikeConfig::is_valid_theme(m_config->get_theme())) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }

    const std::string roguelike_name = m_config->get_theme() + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@StartExplore") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeIterateMonthlySquadPlugin::_run()
{
    LogTraceFunction;

    m_completed = true;
    const int monthly_squad_count = monthlySquadCount[m_config->get_theme()];
    if (monthly_squad_count > 0) {
        ProcessTask(*this, { m_config->get_theme() + "@Roguelike@MonthlySquad" }).run();
    }

    if (!m_iterateMS) {
        m_monthly_squad_index = recognize_monthly_squad_index();
        update_monthly_squad_task();
        apply_monthly_squad_task_strategy();
        return true;
    }

    for (int i = 0; i < monthly_squad_count; i++) {
        m_monthly_squad_index = recognize_monthly_squad_index();
        update_monthly_squad_task();
        apply_monthly_squad_task_strategy();
        if (m_checkComms) {
            ProcessTask(*this, { m_config->get_theme() + "@Roguelike@MonthlySquadComms" }).run();
            if (!try_task("@Roguelike@MonthlySquadCommsCompleted")) {
                try_task("@Roguelike@MonthlySquadCommsBackTwice");
                m_completed = false;
                break;
            }
        }
        bool reward_miss =
            ProcessTask(*this, { m_config->get_theme() + "@Roguelike@MonthlySquadRewardCompleted" }).run();
        if (!reward_miss) {
            m_completed = false;
            break;
        }
    }
    if (m_completed) {
        callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("MonthlySquadCompleted"));
        m_task_ptr->set_enable(false);
    }

    return true;
}

void asst::RoguelikeIterateMonthlySquadPlugin::update_monthly_squad_task()
{
    auto task = RoguelikeMonthlySquad.get_task(m_config->get_theme(), m_monthly_squad_index);
    auto& current_task = m_config->get_monthly_squad_task();
    if (!task.has_value() && !m_monthly_squad_index.has_value() && current_task.has_value() &&
        current_task->theme == m_config->get_theme()) {
        LogWarn << __FUNCTION__ << "keep current monthly squad task after index recognition failed";
        return;
    }
    if (task.has_value() && current_task.has_value() && task->theme == current_task->theme &&
        task->squad_key == current_task->squad_key) {
        task->completed_count = current_task->completed_count;
    }
    m_config->set_monthly_squad_task(std::move(task));
}

void asst::RoguelikeIterateMonthlySquadPlugin::apply_monthly_squad_task_strategy() const
{
    const auto& monthly_squad_task = m_config->get_monthly_squad_task();
    if (!monthly_squad_task.has_value()) {
        return;
    }

    const std::string strategy_task = m_config->get_theme() + "@Roguelike@StrategyChange";
    const std::string strategy_base = strategy_task +
                                      (monthly_squad_task->type == MonthlySquadTaskType::ReachThirdFloor
                                           ? "_mode4"
                                           : "_mode6");
    if (Task.get(strategy_base) == nullptr) {
        LogError << __FUNCTION__ << "monthly squad strategy does not exist:" << strategy_base;
        return;
    }

    Task.set_task_base(strategy_task, strategy_base);
}

std::optional<int> asst::RoguelikeIterateMonthlySquadPlugin::recognize_monthly_squad_index() const
{
    const std::string task_name = m_config->get_theme() + "@Roguelike@MonthlySquadIndex";
    const auto task_info = Task.get<OcrTaskInfo>(task_name);
    if (task_info == nullptr) {
        return std::nullopt;
    }

    OCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info(task_info);
    const auto results = analyzer.analyze();
    if (!results.has_value() || results->size() != 1) {
        LogWarn << __FUNCTION__ << "failed to recognize monthly squad index";
        return std::nullopt;
    }

    int index = 0;
    const std::string& text = results->front().text;
    if (!utils::chars_to_number(text, index) || index < 1 || index > monthlySquadCount.at(m_config->get_theme())) {
        LogWarn << __FUNCTION__ << "invalid monthly squad index:" << text;
        return std::nullopt;
    }

    LogInfo << __FUNCTION__ << "monthly squad index:" << index;
    return index;
}

bool asst::RoguelikeIterateMonthlySquadPlugin::try_task(const char* task) const
{
    return ProcessTask(*this, { m_config->get_theme() + task }).set_times_limit("Roguelike@StartExplore", 0).run();
}
