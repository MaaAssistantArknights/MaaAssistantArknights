#include "RoguelikeSkillSelectionTaskPlugin.h"

#include "Config/Roguelike/RoguelikeRecruitConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Utils/Logger.hpp"
#include "Vision/Roguelike/RoguelikeSkillSelectionImageAnalyzer.h"

bool asst::RoguelikeSkillSelectionTaskPlugin::verify(AsstMsg msg, const json::value& details) const
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
    if (task_view == "Roguelike@StartAction") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeSkillSelectionTaskPlugin::_run()
{
    LogTraceFunction;

    auto image = ctrler()->get_image();
    RoguelikeSkillSelectionImageAnalyzer analyzer(image);

    if (!analyzer.analyze()) {
        return false;
    }

    int delay = Task.get("RoguelikeSkillSelectionMove1")->post_delay;
    bool has_rookie = false;
    for (const auto& [name, skill_vec] : analyzer.get_result()) {
        const auto& oper_info = RoguelikeRecruit.get_oper_info(m_config->get_theme(), name);
        const auto& monthly_squad_task = m_config->get_monthly_squad_task();
        const bool override_skill =
            monthly_squad_task.has_value() &&
            (monthly_squad_task->type == MonthlySquadTaskType::UseOperatorSkill ||
             monthly_squad_task->type == MonthlySquadTaskType::DeployOperatorSummon) &&
            monthly_squad_task->oper_name == name && monthly_squad_task->skill.has_value() &&
            monthly_squad_task->completed_count < monthly_squad_task->required_count;
        if (oper_info.name.empty() && !override_skill) {
            Log.warn("Unknown oper", name);
            continue;
        }

        if (override_skill) {
            const int skill = static_cast<int>(*monthly_squad_task->skill);
            LogInfo << __FUNCTION__ << name << "select monthly squad task skill:" << skill;
            ctrler()->click(skill_vec.at(skill - 1));
            sleep(delay);
        }
        else {
            if (oper_info.alternate_skill > 0) {
                Log.info(__FUNCTION__, name, " select alternate skill:", oper_info.alternate_skill);
                ctrler()->click(skill_vec.at(oper_info.alternate_skill - 1));
                sleep(delay);
            }
            if (oper_info.skill > 0) {
                Log.info(__FUNCTION__, name, " select main skill:", oper_info.skill);
                ctrler()->click(skill_vec.at(oper_info.skill - 1));
                sleep(delay);
            }
        }
        constexpr int RookieStd = 200;
        if (!oper_info.name.empty() && oper_info.promote_priority < RookieStd) {
            has_rookie = true;
        }
    }

    if (m_config->status().opers.empty()) {
        std::unordered_map<std::string, RoguelikeOper> opers;
        for (const auto& [name, skill_vec] : analyzer.get_result()) {
            opers[name] = { .elite = 1, .level = 80 };
            // 不知道是啥等级随便填一个
        }
        m_config->status().opers = std::move(opers);
    }

    if (analyzer.get_team_full() && !has_rookie) {
        Log.info("Team full and no rookie");
        m_config->status().team_full_without_rookie = true;
    }
    else {
        Log.info("Team not full or has rookie");
        m_config->status().team_full_without_rookie = false;
    }
    return true;
}
