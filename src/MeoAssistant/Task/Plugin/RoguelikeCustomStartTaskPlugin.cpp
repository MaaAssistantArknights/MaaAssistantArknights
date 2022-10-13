#include "RoguelikeCustomStartTaskPlugin.h"

#include "../Sub/ProcessTask.h"
#include "Controller.h"
#include "ImageAnalyzer/General/OcrImageAnalyzer.h"
#include "Resource/BattleDataConfiger.h"
#include "RuntimeStatus.h"
#include "TaskData.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeCustomStartTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    auto roguelike_name_opt = m_status->get_properties("roguelike_name");
    if (!roguelike_name_opt) {
        return false;
    }
    const auto& roguelike_name = roguelike_name_opt.value() + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    static const std::unordered_map<std::string_view, std::pair<AsstMsg, RoguelikeCustomType>> TaskMap = {
        { "Roguelike@Recruit1", { AsstMsg::SubTaskCompleted, RoguelikeCustomType::Squad } },
        { "Roguelike@Team3", { AsstMsg::SubTaskCompleted, RoguelikeCustomType::Roles } },
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
    constexpr size_t SwipeTimes = 5;
    for (size_t i = 0; i != SwipeTimes; ++i) {
        auto image = m_ctrler->get_image();
        OcrImageAnalyzer analyzer(image);
        analyzer.set_task_info("RoguelikeCustom-HijackSquad");
        analyzer.set_required({ m_customs[RoguelikeCustomType::Squad] });

        if (!analyzer.analyze()) {
            ProcessTask(*this, { "SlowlySwipeToTheRight" }).run();
            sleep(Task.get("RoguelikeCustom-HijackSquad")->rear_delay);
            continue;
        }
        const auto& rect = analyzer.get_result().front().rect;
        m_ctrler->click(rect);
        return true;
    }
    ProcessTask(*this, { "SwipeToTheLeft" }).run();
    return false;
}

bool asst::RoguelikeCustomStartTaskPlugin::hijack_roles()
{
    auto image = m_ctrler->get_image();
    OcrImageAnalyzer analyzer(image);
    analyzer.set_task_info("RoguelikeCustom-HijackRoles");
    analyzer.set_required({ m_customs[RoguelikeCustomType::Roles] });

    if (!analyzer.analyze()) {
        return false;
    }
    const auto& rect = analyzer.get_result().front().rect;
    m_ctrler->click(rect);
    return true;
}

bool asst::RoguelikeCustomStartTaskPlugin::hijack_core_char()
{
    const std::string& char_name = m_customs[RoguelikeCustomType::CoreChar];

    static const std::unordered_map<BattleRole, std::string> RoleOcrNameMap = {
        { BattleRole::Caster, "术师" }, { BattleRole::Medic, "医疗" },   { BattleRole::Pioneer, "先锋" },
        { BattleRole::Sniper, "狙击" }, { BattleRole::Special, "特种" }, { BattleRole::Support, "辅助" },
        { BattleRole::Tank, "重装" },   { BattleRole::Warrior, "近卫" }
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
    auto image = m_ctrler->get_image();
    OcrImageAnalyzer analyzer(image);
    analyzer.set_task_info("RoguelikeCustom-HijackCoChar");
    analyzer.set_required({ role_ocr_name });
    if (!analyzer.analyze()) {
        return false;
    }
    const auto& role_rect = analyzer.get_result().front().rect;
    m_ctrler->click(role_rect);

    sleep(Task.get("RoguelikeCustom-HijackCoChar")->pre_delay);

    m_status->set_str(RuntimeStatus::RoguelikeCoreChar, char_name);
    return true;
}
