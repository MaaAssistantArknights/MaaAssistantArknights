#include "RoguelikeCustomStartTaskPlugin.h"
#include "OcrImageAnalyzer.h"
#include "TaskData.h"
#include "Controller.h"

bool asst::RoguelikeCustomStartTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted
        || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    static const std::unordered_map<std::string, RoguelikeCustomType> TaskMap = {
        { "Roguelike1Recruit1", RoguelikeCustomType::Squad },
        { "Roguelike1Team3", RoguelikeCustomType::Roles },
        { "Roguelike1RecruitMain", RoguelikeCustomType::CoreChar },
    };
    const std::string& task = details.at("details").at("task").as_string();
    auto it = TaskMap.find(task);
    if (it == TaskMap.cend()) {
        return false;
    }
    auto type = it->second;
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
    auto image = m_ctrler->get_image();
    OcrImageAnalyzer analyzer(image);
    analyzer.set_task_info("Roguelike1Custom-HijackSquad");
    analyzer.set_required({ m_customs[RoguelikeCustomType::Squad] });

    if (!analyzer.analyze()) {
        // TODO: swipe page
        return false;
    }
    const auto& rect = analyzer.get_result().front().rect;
    m_ctrler->click(rect);
    return true;
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

// not work yet
bool asst::RoguelikeCustomStartTaskPlugin::hijack_core_char()
{
    const auto& value = m_customs[RoguelikeCustomType::CoreChar];
    size_t pos = value.find(':');
    if (pos == std::string::npos) {
        return false;
    }
    const auto& role = value.substr(0, pos);
    const auto& char_name = value.substr(pos + 1);

    // select role
    auto image = m_ctrler->get_image();
    OcrImageAnalyzer analyzer(image);
    analyzer.set_task_info("Roguelike1Custom-HijackCoChar");
    analyzer.set_required({ role });
    if (!analyzer.analyze()) {
        return false;
    }
    const auto& role_rect = analyzer.get_result().front().rect;
    m_ctrler->click(role_rect);

    sleep(Task.get("Roguelike1Custom-HijackCoChar")->pre_delay);

    // select core char
    image = m_ctrler->get_image();
    analyzer.set_image(image);
    analyzer.set_required({ char_name });
    if (!analyzer.analyze()) {
        // Todo: swipe page
        return false;
    }
    const auto& char_rect = analyzer.get_result().front().rect;
    m_ctrler->click(char_rect);

    return true;
}
