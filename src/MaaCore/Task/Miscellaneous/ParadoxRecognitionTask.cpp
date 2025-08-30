#include "ParadoxRecognitionTask.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::ParadoxRecognitionTask::_run()
{
    LogTraceFunction;
    const auto& all_oper_names = BattleData.get_all_oper_names();

    for (const auto& name : all_oper_names) {
        if (BattleData.get_id(name).ends_with(m_navigate_name)) { // 应该没重复吧
            m_oper_name = json::object {
                { "name", name },
                { "name_en", BattleData.get_en(name) },
                { "name_jp", BattleData.get_jp(name) },
                { "name_kr", BattleData.get_kr(name) },
                { "name_tw", BattleData.get_tw(name) },
            };
            break;
        }
    }

    // 设置技能
    m_skill_num = 1;
    auto* groups = &Copilot.get_data().groups;
    for (const auto& opers_vec : *groups | views::values) {
        if (opers_vec.empty()) {
            continue;
        }
        for (const auto& oper : opers_vec) {
            if (match_oper(oper.name)) {
                m_skill_num = oper.skill;
            }
        }
    }

    return_initial_oper(); // 回干员列表（默认在最左侧）
    const auto role = BattleData.get_role(m_oper_name["name"].as_string()); 
    if (!click_role_table(role)) {
        return_initial_oper();
    }

    if (swipe_and_analyze()) {
        enter_paradox(m_skill_num);
    }

    return true;
}

std::string asst::ParadoxRecognitionTask::standardize_name(const std::string& navigate_name)
{
    size_t length = navigate_name.length();
    return navigate_name.substr(4, length - 6);
}

void asst::ParadoxRecognitionTask::enter_paradox(int skill_num) const
{
    ctrler()->click(m_navigate_rect);
    ProcessTask(*this, { "OperParadoxBegin" }).run();
    ProcessTask(*this, { "ParadoxChooseSkill" + std::to_string(skill_num) }).run();
}

void asst::ParadoxRecognitionTask::set_navigate_name(const std::string& navigate_name)
{
    m_navigate_name = standardize_name(navigate_name);
}

void asst::ParadoxRecognitionTask::swipe_page() const
{
    ProcessTask(*this, { "OperBoxSlowlySwipeToTheRight" }).run();
}

void asst::ParadoxRecognitionTask::return_initial_oper() const
{
    if (!ProcessTask(*this, { "ParadoxReturnOperListFlag" }).set_retry_times(0).run()) {
        ProcessTask(*this, { "ParadoxReturnUntilOperList" }).run();
    }
    ProcessTask(*this, { "BattleQuickFormationExpandRole" }).set_retry_times(3).run();
    ProcessTask(*this, { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" }).run();
    ProcessTask(
        *this,
        { "BattleQuickFormationRole-Pioneer",
          "BattleQuickFormationRole-Warrior",
          "BattleQuickFormationRole-Tank",
          "BattleQuickFormationRole-Caster",
          "BattleQuickFormationRole-Medic",
          "BattleQuickFormationRole-Sniper",
          "BattleQuickFormationRole-Special",
          "BattleQuickFormationRole-Support" })
        .run();
    ProcessTask(*this, { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" }).run();
}

bool asst::ParadoxRecognitionTask::click_role_table(const battle::Role role) const
{
    static const std::unordered_map<battle::Role, std::string> role_name_type = {
        { battle::Role::Caster, "Caster" }, { battle::Role::Medic, "Medic" },     { battle::Role::Pioneer, "Pioneer" },
        { battle::Role::Sniper, "Sniper" }, { battle::Role::Special, "Special" }, { battle::Role::Support, "Support" },
        { battle::Role::Tank, "Tank" },     { battle::Role::Warrior, "Warrior" },
    };

    const auto role_iter = role_name_type.find(role);

    if (role_iter == role_name_type.cend()) {
        return false;
    }

    ProcessTask(*this, { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" }).run();
    return ProcessTask(*this, { "BattleQuickFormationRole-" + role_iter->second }).set_retry_times(0).run();
}

bool asst::ParadoxRecognitionTask::swipe_and_analyze()
{
    LogTraceFunction;
    std::string pre_pre_last_oper;
    std::string pre_last_oper;

    bool find_oper = false;

    while (!need_exit()) {
        OperBoxImageAnalyzer analyzer(ctrler()->get_image());

        if (!analyzer.analyze()) {
            break;
        }
        const auto& opers_result = analyzer.get_result();

        const std::string& last_oper = opers_result.back().name;
        if (last_oper == pre_last_oper && pre_last_oper == pre_pre_last_oper) {
            break;
        }
        pre_pre_last_oper = pre_last_oper;
        pre_last_oper = last_oper;

        for (const auto& box_info : opers_result) {
            if (match_oper(box_info.name)) {
                m_navigate_rect = box_info.rect;
                find_oper = true;
                break;
            }
        }

        if (find_oper) {
            break;
        }

        swipe_page();
    }
    return find_oper;
}

bool asst::ParadoxRecognitionTask::match_oper(const std::string& name)
{
    return m_oper_name["name"] == name || m_oper_name["name_en"] == name || m_oper_name["name_jp"] == name ||
           m_oper_name["name_kr"] == name || m_oper_name["name_tw"] == name;
}
