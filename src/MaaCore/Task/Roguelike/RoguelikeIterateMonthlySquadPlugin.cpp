#include "RoguelikeIterateMonthlySquadPlugin.h"

#include "Config/GeneralConfig.h"
#include "Config/Roguelike/RoguelikeMonthlySquadConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

bool asst::RoguelikeIterateMonthlySquadPlugin::load_params([[maybe_unused]] const json::value& params)
{
    m_checkComms = params.find<bool>("monthly_squad_check_comms").value_or(false);

    m_iterateMS = params.find<bool>("monthly_squad_auto_iterate").value_or(false);
    m_use_legacy_monthly_squad_logic = false;
    m_legacy_monthly_squad_index.reset();
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
    const bool squad_page_opened =
        monthly_squad_count > 0 && ProcessTask(*this, { m_config->get_theme() + "@Roguelike@MonthlySquad" }).run();

    const int squads_to_check = m_iterateMS ? monthly_squad_count : 1;
    for (int i = 0; i < squads_to_check; i++) {
        m_monthly_squad_index = recognize_monthly_squad_index(i > 0);
        if (m_use_legacy_monthly_squad_logic && m_legacy_monthly_squad_index.has_value() &&
            m_monthly_squad_index.has_value() && m_legacy_monthly_squad_index != m_monthly_squad_index) {
            m_use_legacy_monthly_squad_logic = false;
            m_legacy_monthly_squad_index.reset();
        }
        if (m_use_legacy_monthly_squad_logic) {
            apply_legacy_monthly_squad_logic();
        }
        else {
            update_monthly_squad_task();
            apply_monthly_squad_task_strategy();
        }

        auto& task = m_config->get_monthly_squad_task();
        const bool recheck_count = task && task->required_count > 0 && task->completed_count >= task->required_count &&
                                   (m_monthly_squad_index || (squad_page_opened && task->squad_key == "default"));
        const bool reward_completed =
            (!m_iterateMS || m_checkComms || recheck_count) && is_monthly_squad_reward_completed();
        if (recheck_count && !reward_completed && !need_exit()) {
            LogWarn << __FUNCTION__ << "monthly squad reward incomplete, resetting local progress:"
                    << task->theme << task->squad_key << task->completed_count << "/" << task->required_count;
            task->completed_count = 0;
        }
        if (!m_iterateMS) {
            if (reward_completed) {
                LogInfo << __FUNCTION__ << "current monthly squad reward completed, finishing Roguelike task";
                m_task_ptr->set_enable(false);
            }
            return true;
        }
        if (m_checkComms) {
            ProcessTask(*this, { m_config->get_theme() + "@Roguelike@MonthlySquadComms" }).run();
            if (!try_task("@Roguelike@MonthlySquadCommsCompleted")) {
                if (reward_completed) {
                    apply_legacy_monthly_squad_logic();
                }
                try_task("@Roguelike@MonthlySquadCommsBackTwice");
                m_completed = false;
                break;
            }
        }
        const bool switched_squad =
            ProcessTask(*this, { m_config->get_theme() + "@Roguelike@MonthlySquadRewardCompleted" }).run();
        if (!switched_squad) {
            m_completed = false;
            break;
        }
        // 领取状态匹配后会点击切队。下一次识别不能沿用上一队的目标和进度。
        m_config->set_monthly_squad_task(std::nullopt);
        m_use_legacy_monthly_squad_logic = false;
        m_legacy_monthly_squad_index.reset();
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
    const std::string prefix = m_config->get_theme() + "@Roguelike@";
    const std::string strategy_task = prefix + "StrategyChange";
    const bool reach_third_floor =
        monthly_squad_task && monthly_squad_task->type == MonthlySquadTaskType::ReachThirdFloor;
    const std::string strategy_base = strategy_task + (reach_third_floor ? "_ReachThirdFloor" : "_mode6");
    if (Task.get(strategy_base) == nullptr) {
        LogError << __FUNCTION__ << "monthly squad strategy does not exist:" << strategy_base;
        return;
    }

    Task.set_task_base(strategy_task, strategy_base);
    // 开局先恢复默认选点，避免沿用上一局到达第三层时的重开策略。
    const std::string stages_task = prefix + "Stages";
    Task.set_task_base(stages_task, stages_task + "_default");
}

void asst::RoguelikeIterateMonthlySquadPlugin::apply_legacy_monthly_squad_logic()
{
    m_use_legacy_monthly_squad_logic = true;
    m_legacy_monthly_squad_index = m_monthly_squad_index;
    m_config->set_monthly_squad_task(std::nullopt);
    apply_monthly_squad_task_strategy();
    LogInfo << __FUNCTION__ << "use legacy monthly squad logic until communications are completed";
}

bool asst::RoguelikeIterateMonthlySquadPlugin::is_monthly_squad_reward_completed() const
{
    // 刚进入小队页面时，奖励标记可能还在淡入。
    Matcher analyzer(ctrler()->get_image());
    analyzer.set_task_info(m_config->get_theme() + "@Roguelike@MonthlySquadRewardCompleted");
    if (analyzer.analyze()) {
        return true;
    }
    if (!sleep(Config.get_options().task_delay)) {
        return false;
    }
    analyzer.set_image(ctrler()->get_image());
    return analyzer.analyze().has_value();
}

std::optional<int> asst::RoguelikeIterateMonthlySquadPlugin::recognize_monthly_squad_index(bool after_switch) const
{
    const std::string task_name = m_config->get_theme() + "@Roguelike@MonthlySquadIndex";
    const auto task_info = Task.get<OcrTaskInfo>(task_name);
    if (task_info == nullptr) {
        return std::nullopt;
    }

    const int max_attempts = after_switch ? 3 : 1;
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        if (attempt > 0 && !sleep(Config.get_options().task_delay)) {
            return std::nullopt;
        }
        OCRer analyzer(ctrler()->get_image());
        analyzer.set_task_info(task_info);
        const auto results = analyzer.analyze();
        if (!results || results->size() != 1) {
            continue;
        }

        int index = 0;
        const std::string& text = results->front().text;
        if (!utils::chars_to_number(text, index) || index < 1 || index > monthlySquadCount.at(m_config->get_theme())) {
            LogWarn << __FUNCTION__ << "invalid monthly squad index:" << text;
            continue;
        }
        if (after_switch && index == m_monthly_squad_index) {
            continue; // 切队动画中仍可能读到上一队的编号。
        }
        LogInfo << __FUNCTION__ << "monthly squad index:" << index;
        return index;
    }
    LogWarn << __FUNCTION__ << "failed to recognize monthly squad index";
    return std::nullopt;
}

bool asst::RoguelikeIterateMonthlySquadPlugin::try_task(const char* task) const
{
    return ProcessTask(*this, { m_config->get_theme() + task }).set_times_limit("Roguelike@StartExplore", 0).run();
}
