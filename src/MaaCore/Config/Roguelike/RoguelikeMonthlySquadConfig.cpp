#include "RoguelikeMonthlySquadConfig.h"

#include <unordered_set>
#include <utility>

#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

std::optional<asst::MonthlySquadTask> asst::RoguelikeMonthlySquadConfig::get_task(
    const std::string& theme,
    const std::optional<int>& squad_index) const
{
    const auto theme_iter = m_tasks.find(theme);
    if (theme_iter == m_tasks.cend()) {
        return std::nullopt;
    }

    if (squad_index.has_value()) {
        const auto task_iter = theme_iter->second.find(std::to_string(*squad_index));
        if (task_iter != theme_iter->second.cend()) {
            return task_iter->second;
        }
    }

    const auto default_iter = theme_iter->second.find("default");
    return default_iter == theme_iter->second.cend() ? std::nullopt
                                                     : std::optional<MonthlySquadTask>(default_iter->second);
}

bool asst::RoguelikeMonthlySquadConfig::parse(const json::value& json)
{
    LogTraceFunction;

    if (!json.is_object()) {
        LogError << __FUNCTION__ << "monthly squad config must be an object";
        return false;
    }

    const std::string theme = m_path.parent_path().filename().string();
    static const std::unordered_set<std::string> MonthlySquadThemes = {
        "Phantom",
        "Mizuki",
        "Sami",
        "Sarkaz",
        "JieGarden",
    };
    if (!MonthlySquadThemes.contains(theme)) {
        LogError << __FUNCTION__ << "invalid monthly squad theme:" << theme;
        return false;
    }

    std::unordered_map<std::string, MonthlySquadTask> tasks;
    for (const auto& [squad_key, task_json] : json.as_object()) {
        if (!task_json.is_object()) {
            LogError << __FUNCTION__ << "monthly squad task must be an object, theme:" << theme
                     << "squad:" << squad_key;
            return false;
        }

        int squad_index = 0;
        if (squad_key != "default" &&
            (!utils::chars_to_number(squad_key, squad_index) || squad_index < 1 || squad_index > 8)) {
            LogError << __FUNCTION__ << "invalid monthly squad key, theme:" << theme << "squad:" << squad_key;
            return false;
        }

        MonthlySquadTask task;
        task.theme = theme;
        task.squad_key = squad_key;
        const std::string type = task_json.get("type", "");
        if (type == "ReachThirdFloor") {
            task.type = MonthlySquadTaskType::ReachThirdFloor;
        }
        else if (type == "DeployOperator") {
            task.type = MonthlySquadTaskType::DeployOperator;
        }
        else if (type == "DeployOperatorSummon") {
            task.type = MonthlySquadTaskType::DeployOperatorSummon;
        }
        else if (type == "UseOperatorSkill") {
            task.type = MonthlySquadTaskType::UseOperatorSkill;
        }
        else {
            LogError << __FUNCTION__ << "invalid monthly squad task type:" << type << "theme:" << theme
                     << "squad:" << squad_key;
            return false;
        }

        if (task.type == MonthlySquadTaskType::ReachThirdFloor) {
            tasks.emplace(squad_key, std::move(task));
            continue;
        }

        task.oper_name = task_json.get("operator", "");
        task.required_count = task_json.get("count", 0);
        if (task.oper_name.empty() || task.required_count <= 0) {
            LogError << __FUNCTION__ << "monthly squad task requires a valid operator and count, theme:" << theme
                     << "squad:" << squad_key;
            return false;
        }

        if (task.type == MonthlySquadTaskType::DeployOperatorSummon) {
            task.summon_name = task_json.get("summon", "");
        }
        else if (task.type == MonthlySquadTaskType::UseOperatorSkill) {
            const int skill = task_json.get("skill", 0);
            if (skill < 1 || skill > 3) {
                LogError << __FUNCTION__ << "monthly squad skill must be 1, 2, or 3, theme:" << theme
                         << "squad:" << squad_key;
                return false;
            }
            task.skill = static_cast<MonthlySquadSkill>(skill);
        }

        tasks.emplace(squad_key, std::move(task));
    }

    m_tasks.insert_or_assign(theme, std::move(tasks));
    return true;
}
