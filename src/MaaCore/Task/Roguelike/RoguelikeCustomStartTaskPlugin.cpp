#include "RoguelikeCustomStartTaskPlugin.h"

#include "Controller.h"
#include "Vision/OcrImageAnalyzer.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeCustomStartTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    auto roguelike_name_opt = status()->get_properties(Status::RoguelikeTheme);
    if (!roguelike_name_opt) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = std::move(roguelike_name_opt.value()) + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    static const std::unordered_map<std::string_view, std::pair<AsstMsg, RoguelikeCustomType>> TaskMap = {
        { "Roguelike@SquadDefault", { AsstMsg::SubTaskCompleted, RoguelikeCustomType::Squad } },
        { "Roguelike@RolesDefault", { AsstMsg::SubTaskCompleted, RoguelikeCustomType::Roles } },
        { "Roguelike@RecruitMain", { AsstMsg::SubTaskStart, RoguelikeCustomType::CoreChar } },
    };
    auto it = TaskMap.find(task_view);
    if (it == TaskMap.cend()) {
        return false;
    }
    if (msg != it->second.first) {
        return false;
    }
    auto type = it->second.second;
    if (auto custom_it = m_customs.find(type); custom_it == m_customs.cend() || custom_it->second.empty()) {
        return false;
    }

    m_waiting_to_run = type;
    return true;
}

void asst::RoguelikeCustomStartTaskPlugin::set_custom(RoguelikeCustomType type, std::string custom)
{
    m_customs.insert_or_assign(type, std::move(custom));
}

bool asst::RoguelikeCustomStartTaskPlugin::_run()
{
    const std::unordered_map<RoguelikeCustomType, std::function<bool(void)>> TypeActuator = {
        { RoguelikeCustomType::Squad, std::bind(&RoguelikeCustomStartTaskPlugin::hijack_squad, this) },
        { RoguelikeCustomType::Roles, std::bind(&RoguelikeCustomStartTaskPlugin::hijack_roles, this) },
        { RoguelikeCustomType::CoreChar, std::bind(&RoguelikeCustomStartTaskPlugin::hijack_core_char, this) },
    };

    auto it = TypeActuator.find(m_waiting_to_run);
    if (it == TypeActuator.cend()) {
        return false;
    }

    if (auto custom_it = m_customs.find(it->first); custom_it == m_customs.cend()) {
        return false;
    }
    else if (custom_it->second.empty()) {
        return false;
    }

    return it->second();
}

bool asst::RoguelikeCustomStartTaskPlugin::hijack_squad()
{
    constexpr size_t SwipeTimes = 7;
    for (size_t i = 0; i != SwipeTimes; ++i) {
        auto image = ctrler()->get_image();
        OcrImageAnalyzer analyzer(image);
        analyzer.set_task_info("RoguelikeCustom-HijackSquad");
        analyzer.set_required({ m_customs[RoguelikeCustomType::Squad] });

        if (!analyzer.analyze()) {
            ProcessTask(*this, { "SlowlySwipeToTheRight" }).run();
            sleep(Task.get("RoguelikeCustom-HijackSquad")->post_delay);
            continue;
        }
        const auto& rect = analyzer.get_result().front().rect;
        ctrler()->click(rect);
        return true;
    }
    ProcessTask(*this, { "SwipeToTheLeft" }).run();
    return false;
}

bool asst::RoguelikeCustomStartTaskPlugin::hijack_roles()
{
    auto image = ctrler()->get_image();
    OcrImageAnalyzer analyzer(image);
    analyzer.set_task_info("RoguelikeCustom-HijackRoles");
    analyzer.set_required({ m_customs[RoguelikeCustomType::Roles] });

    if (!analyzer.analyze()) {
        return false;
    }
    const auto& rect = analyzer.get_result().front().rect;
    ctrler()->click(rect);
    return true;
}

bool asst::RoguelikeCustomStartTaskPlugin::hijack_core_char()
{
    const std::string& char_name = m_customs[RoguelikeCustomType::CoreChar];

    static const std::unordered_map<battle::Role, std::string> RoleOcrNameMap = {
        { battle::Role::Caster, "术师" }, { battle::Role::Medic, "医疗" },   { battle::Role::Pioneer, "先锋" },
        { battle::Role::Sniper, "狙击" }, { battle::Role::Special, "特种" }, { battle::Role::Support, "辅助" },
        { battle::Role::Tank, "重装" },   { battle::Role::Warrior, "近卫" }
    };
    const auto& role = BattleData.get_role(char_name);
    auto role_iter = RoleOcrNameMap.find(role);
    if (role_iter == RoleOcrNameMap.cend()) {
        Log.error("Unknown role", char_name, static_cast<int>(role));
        return false;
    }
    // select role
    const std::string& role_ocr_name = role_iter->second;
    Log.info("role", role_ocr_name);
    auto image = ctrler()->get_image();
    OcrImageAnalyzer analyzer(image);
    analyzer.set_task_info("RoguelikeCustom-HijackCoChar");
    analyzer.set_required({ role_ocr_name });
    if (!analyzer.analyze()) {
        return false;
    }
    const auto& role_rect = analyzer.get_result().front().rect;
    ctrler()->click(role_rect);

    sleep(Task.get("RoguelikeCustom-HijackCoChar")->pre_delay);

    status()->set_str(Status::RoguelikeUseSupport, m_customs[RoguelikeCustomType::UseSupport]);
    status()->set_str(Status::RoguelikeUseNonfriendSupport, m_customs[RoguelikeCustomType::UseNonfriendSupport]);
    status()->set_str(Status::RoguelikeCoreChar, char_name);
    return true;
}
