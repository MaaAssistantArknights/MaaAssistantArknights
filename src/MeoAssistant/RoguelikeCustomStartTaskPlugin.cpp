#include "RoguelikeCustomStartTaskPlugin.h"
#include "OcrImageAnalyzer.h"
#include "TaskData.h"
#include "Controller.h"
#include "ProcessTask.h"
#include "Resource.h"
#include "Logger.hpp"
#include "RuntimeStatus.h"

bool asst::RoguelikeCustomStartTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    static const std::unordered_map<std::string, std::pair<AsstMsg, RoguelikeCustomType>> TaskMap = {
        { "Roguelike1Recruit1", { AsstMsg::SubTaskCompleted, RoguelikeCustomType::Squad } },
        { "Roguelike1Team3",  { AsstMsg::SubTaskCompleted, RoguelikeCustomType::Roles } },
        { "Roguelike1RecruitMain", { AsstMsg::SubTaskStart, RoguelikeCustomType::CoreChar} },
    };
    const std::string& task = details.get("details", "task", "");
    auto it = TaskMap.find(task);
    if (it == TaskMap.cend()) {
        return false;
    }
    if (msg != it->second.first) {
        return false;
    }
    auto type = it->second.second;
    if (auto custom_it = m_customs.find(type);
        custom_it == m_customs.cend() || custom_it->second.empty()) {
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

    if (auto custom_it = m_customs.find(it->first);
        custom_it == m_customs.cend()) {
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
        analyzer.set_task_info("Roguelike1Custom-HijackSquad");
        analyzer.set_required({ m_customs[RoguelikeCustomType::Squad] });

        if (!analyzer.analyze()) {
            ProcessTask(*this, { "SlowlySwipeToTheRight" }).run();
            sleep(Task.get("Roguelike1Custom-HijackSquad")->rear_delay);
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
    analyzer.set_task_info("Roguelike1Custom-HijackRoles");
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
        { BattleRole::Caster, "术师" },
        { BattleRole::Medic, "医疗" },
        { BattleRole::Pioneer, "先锋" },
        { BattleRole::Sniper, "狙击" },
        { BattleRole::Special, "特种" },
        { BattleRole::Support, "辅助" },
        { BattleRole::Tank, "重装" },
        { BattleRole::Warrior, "近卫" }
    };
    const auto& role = Resrc.battle_data().get_role(char_name);
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
    analyzer.set_task_info("Roguelike1Custom-HijackCoChar");
    analyzer.set_required({ role_ocr_name });
    if (!analyzer.analyze()) {
        return false;
    }
    const auto& role_rect = analyzer.get_result().front().rect;
    m_ctrler->click(role_rect);

    sleep(Task.get("Roguelike1Custom-HijackCoChar")->pre_delay);

    m_status->set_str("RoguelikeCoreChar", char_name);
    return true;
}
