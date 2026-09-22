#include "OperProgressTask.h"

#include <ranges>

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Utils/Logger.hpp"

asst::OperProgressTask::OperProgressTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_process_task_ptr(std::make_shared<AutoRaiseProcessTask>(callback, inst, TaskType))
{
    m_process_task_ptr->set_retry_times(0);
    m_subtasks.emplace_back(m_process_task_ptr);
}

bool asst::OperProgressTask::set_params(const json::value& params)
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
class jsonization<asst::OperProgressTask::ProgressTargetDto>
{
public:
    bool check_json(const json::value& json) const
    {
        static constexpr std::array<const char*, 4> allowed_keys = {
            "role",
            "name",
            "elite",
            "skill_level",
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
        const auto check_field = [&]<typename T>(const char* key, bool required = true) -> std::optional<T> {
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
        // 养成动作一律是整数，显式写 null 时 is<int>() 为假，与类型错误同等拒绝，不会被当成未配置。
        const auto& role_opt = check_field.template operator()<asst::battle::Role>("role", false);
        const auto& name_opt = check_field.template operator()<std::string>("name", true);
        const auto& elite_opt = check_field.template operator()<int>("elite", false);
        const auto& skill_level_opt =
            check_field.template operator()<std::variant<int, std::array<int, 3>>>("skill_level", false);

        if (!ret) {
            return false;
        }
        if (name_opt->empty() || std::ranges::all_of(*name_opt, [](unsigned char ch) { return std::isspace(ch); })) {
            LogError << __FUNCTION__ << "name must be non-empty";
            return false;
        }

        if (elite_opt && (*elite_opt < 1 || *elite_opt > 2)) {
            LogError << __FUNCTION__ << "elite must be 1 or 2";
            return false;
        }
        if (!skill_level_opt) {
        }
        else if (auto base_opt = std::get_if<int>(&skill_level_opt.value()); base_opt != nullptr) {
            if (*base_opt < 2 || *base_opt > 7) {
                LogError << __FUNCTION__ << "skill_level must be between 2 and 7";
                return false;
            }
        }
        else if (
            auto specialization_opt = std::get_if<std::array<int, 3>>(&skill_level_opt.value());
            specialization_opt != nullptr) {
            if (std::ranges::any_of(*specialization_opt, [](int level) { return level < 0 || level > 3; })) {
                LogError << __FUNCTION__ << "skill_level specialization must be between 0 and 3";
                return false;
            }
        }
        if (role_opt && *role_opt == asst::battle::Role::Unknown) {
            const auto& role = asst::BattleData.get_roles(*name_opt, true);
            if (role.empty() || role.size() > 1) {
                LogError << __FUNCTION__ << "oper name:" << *name_opt << "with multi role, and not specific";
                return false;
            }
        }
        else if (asst::BattleData.find_opers(*role_opt, *name_opt).empty()) {
            LogError << __FUNCTION__ << "unknown oper name: " << *name_opt << ", role:" << *role_opt;
            return false;
        }
        return true;
    }
};
} // namespace json::ext

std::optional<asst::AutoRaiseProcessTask::AutoRaisePlan> asst::OperProgressTask::parse_plan(const json::value& params)
{
    const auto& plans = params.find<std::vector<ProgressTargetDto>>("plans");
    if (!plans) {
        LogError << __FUNCTION__ << "missing plans, or format is error";
        return std::nullopt;
    }

    AutoRaiseProcessTask::AutoRaisePlan result;
    result.reserve(plans->size());
    for (const auto& plan : *plans) {
        battle::Role role = plan.role;
        if (role == battle::Role::Unknown) {
            const auto& roles = BattleData.get_roles(plan.name, true);
            if (roles.empty() || roles.size() > 1) {
                LogError << __FUNCTION__ << "oper name:" << plan.name << "with multi role, and not specific";
                return std::nullopt;
            }
            role = *roles.begin();
        }
        if (plan.elite) {
            result.emplace_back(
                AutoRaiseProcessTask::OperProgressTarget {
                    .role = role,
                    .name = plan.name,
                    .action = AutoRaiseProcessTask::OperProgressAction::Elite,
                    .target = *plan.elite,
                });
        }
        // TODO 继续整理
    }
    return result;
}
