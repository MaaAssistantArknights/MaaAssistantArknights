#include "AutoRaiseTask.h"

#include <ranges>

#include "Task/AutoRaise/AutoRaisePlanParser.h"
#include "Task/AutoRaise/AutoRaiseProcessTask.h"
#include "Utils/Logger.hpp"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Task/AutoRaise/AutoRaisePlan.h"

asst::AutoRaiseTask::AutoRaiseTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_process_task_ptr(std::make_shared<AutoRaiseProcessTask>(callback, inst, TaskType))
{
    m_process_task_ptr->set_retry_times(0);
    m_subtasks.emplace_back(m_process_task_ptr);
}

bool asst::AutoRaiseTask::set_params(const json::value& params)
{
    LogTraceFunction;
    auto plan = parse_plan(params);
    if (!plan) {
        LogError << __FUNCTION__ << "invalid plans";
        return false;
    }
    m_process_task_ptr->set_plan(std::move(*plan));
    return true;
}

namespace json::ext
{
template <>
class jsonization<asst::AutoRaiseTask::AutoRaiseTargetDto>
{
public:
    bool check_json(const json::value& json) const
    {
        static constexpr std::array<const char*, 5> allowed_keys = {
            "name", "elite", "skills", "skill", "skill_master",
        };

        if (!json.is_object()) {
            return false;
        }
        const auto& obj = json.as_object();
        for (const auto& kv : obj) {
            if (std::ranges::find(allowed_keys, kv.first) == allowed_keys.end()) {
                return false;
            }
        }

        bool ret = true;
        const auto check_field = [&]<typename T>(const char* key, const T&, bool required = true) -> std::optional<T> {
            const auto& found = json.find_value(key);
            if (!found) {
                ret = ret && !required;
            }
            else if (!found->is<T>()) {
                ret = false;
            }
            else {
                return found->as<T>();
            }
            return std::nullopt;
        };
        static std::string _;
        // 养成动作一律是整数，显式写 null 时 is<int>() 为假，与类型错误同等拒绝，不会被当成未配置。
        const auto& name_opt = check_field("name", _, true);
        const auto& elite_opt = check_field("elite", 0, false);
        const auto& skills_opt = check_field("skills", 0, false);
        const auto& skill_opt = check_field("skill", 0, false);
        const auto& skill_master_opt = check_field("skill_master", 0, false);

        if (!ret) {
            return false;
        }
        if (name_opt->empty() || std::ranges::all_of(*name_opt, [](unsigned char ch) { return std::isspace(ch); })) {
            LogError << __FUNCTION__ << "name must be non-empty";
            return false;
        }
        const int actions = static_cast<int>(elite_opt.has_value()) + static_cast<int>(skills_opt.has_value()) +
                            static_cast<int>(skill_opt.has_value() || skill_master_opt.has_value());
        if (actions != 1 || skill_opt.has_value() != skill_master_opt.has_value()) {
            LogError << __FUNCTION__ << "plan must define exactly one of elite, skills, or skill with skill_master";
            return false;
        }
        if (elite_opt && (*elite_opt < 1 || *elite_opt > 2)) {
            LogError << __FUNCTION__ << "elite must be 1 or 2";
            return false;
        }
        if (skills_opt && (*skills_opt < 2 || *skills_opt > 7)) {
            LogError << __FUNCTION__ << "skills must be between 2 and 7";
            return false;
        }
        if (skill_opt && (*skill_opt < 1 || *skill_opt > 3)) {
            LogError << __FUNCTION__ << "skill must be between 1 and 3";
            return false;
        }
        if (skill_master_opt && (*skill_master_opt < 1 || *skill_master_opt > 3)) {
            LogError << __FUNCTION__ << "skill_master must be between 1 and 3";
            return false;
        }
        return true;
    }
};
} // namespace json::ext

std::optional<asst::AutoRaisePlan> asst::AutoRaiseTask::parse_plan(const json::value& params)
{
    const auto& plans = params.find<std::vector<AutoRaiseTargetDto>>("plans");
    if (!plans) {
        LogError << __FUNCTION__ << "missing plans, or format is error";
        return std::nullopt;
    }

    AutoRaisePlan result;
    result.reserve(plans->size());
    for (const auto& plan : *plans) {
        AutoRaiseTarget target { .name = plan.name };
        if (plan.elite) {
            target.action = AutoRaiseAction::Elite;
            target.target = *plan.elite;
        }
        else if (plan.skills) {
            target.action = AutoRaiseAction::Skills;
            target.target = *plan.skills;
        }
        else {
            target.action = AutoRaiseAction::Mastery;
            target.skill = *plan.skill;
            target.target = *plan.skill_master;
        }
        result.emplace_back(std::move(target));
    }
    return result;
}
