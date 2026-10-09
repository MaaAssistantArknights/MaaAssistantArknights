#include "ParadoxRecognitionTask.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Controller/Controller.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/Miscellaneous/ParadoxListTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/ParadoxDetailAnalyzer.h"

bool asst::ParadoxRecognitionTask::_run()
{
    LogTraceFunction;
    if (m_paradox_files.empty()) {
        LogError << __FUNCTION__ << "no paradox oper set";
        return false;
    }

    const auto& [id, raw_path] = m_paradox_files.front();
    const auto& path = utils::path(raw_path);
    if (!Copilot.load(path)) {
        LogError << "CopilotConfig parse failed";
        return false;
    }
    const auto file_name = utils::path_to_utf8_string(path);
    const auto& stage_name = Copilot.get_stage_name();
    if (!m_battle_task_ptr->set_stage_name(stage_name)) {
        LogError << "Not support stage";
        return false;
    }

    json::value info = basic_info_with_what("CopilotListLoadTaskFileSuccess");
    info["details"]["stage_name"] = stage_name;
    info["details"]["file_name"] = file_name;
    info["details"]["id"] = id;
    callback(AsstMsg::SubTaskExtraInfo, info);

    m_navigate_name = standardize_name(stage_name);
    LogInfo << __FUNCTION__ << "navigate name:" << m_navigate_name;
    m_paradox_files.erase(m_paradox_files.begin());

    const auto& all_oper_names = BattleData.get_all_chars();
    const auto it = std::find_if(all_oper_names.begin(), all_oper_names.end(), [&](const auto& pair) {
        return pair.second && pair.second->role != battle::Role::Drone && pair.first.ends_with(m_navigate_name);
    });
    if (it == all_oper_names.end()) {
        report_status("ParadoxOperatorNotFound");
        return false;
    }
    m_oper_name = {
        it->second->role,    it->second->rarity,  it->second->name,    it->second->name_en,
        it->second->name_jp, it->second->name_kr, it->second->name_tw,
    };

    m_skill_num = 1;
    for (const auto& group : Copilot.get_data().groups) {
        for (const auto& oper : group.opers) {
            if (match_oper(oper.name) && oper.skill >= 1 && oper.skill <= 3) {
                m_skill_num = oper.skill;
            }
        }
    }
    LogInfo << __FUNCTION__ << "operator:" << m_oper_name.name << "rarity:" << m_oper_name.rarity
            << "skill:" << m_skill_num;

    if (m_from_detail) {
        m_from_detail = false;
        ParadoxDetailAnalyzer detail(ctrler()->get_image());
        const auto name = detail.analyze();
        if (!name || *name != m_oper_name.name) {
            report_status(name ? "ParadoxOperatorMismatch" : "ParadoxRecognitionFailed");
            return false;
        }
        return enter_paradox(m_skill_num, m_oper_name.rarity);
    }

    ParadoxListTask locate(m_callback, m_inst, m_task_chain);
    locate.set_task_id(m_task_id).set_retry_times(0);
    locate.set_target(m_oper_name.name);
    if (!locate.run()) {
        report_status("ParadoxRecognitionFailed");
        return false;
    }
    if (!locate.found_target()) {
        report_status("ParadoxOperatorNotFound");
        return false;
    }
    if (locate.target_completed()) {
        report_status("ParadoxAlreadyCompleted");
        return_to_oper_list();
        return false;
    }
    return enter_paradox(m_skill_num, m_oper_name.rarity);
}

std::string asst::ParadoxRecognitionTask::standardize_name(const std::string& navigate_name)
{
    return navigate_name.substr(4, navigate_name.length() - 6);
}

bool asst::ParadoxRecognitionTask::enter_paradox(const int skill_num, const int rarity)
{
    if (ProcessTask(*this, { "ParadoxAlreadyCompleted" }).set_retry_times(0).run()) {
        report_status("ParadoxAlreadyCompleted");
        return_to_oper_list();
        return false;
    }
    if (!ProcessTask(*this, { "ParadoxStartSimulation" }).set_retry_times(1).run()) {
        if (!ProcessTask(*this, { "OperParadoxBegin" }).set_retry_times(3).run()) {
            report_status("ParadoxRecognitionFailed");
            return_to_oper_list();
            return false;
        }
        if (ProcessTask(*this, { "ParadoxAlreadyCompleted" }).set_retry_times(0).run()) {
            report_status("ParadoxAlreadyCompleted");
            return_to_oper_list();
            return false;
        }
        if (!ProcessTask(*this, { "OperOpenParadoxChooseSkill" }).set_retry_times(3).run()) {
            report_status("ParadoxSkillSelectFailed");
            return_to_oper_list();
            return false;
        }
    }
    if (rarity > 2) {
        if (!ProcessTask(*this, { "ParadoxChooseSkill" + std::to_string(skill_num) }).set_retry_times(3).run()) {
            report_status("ParadoxSkillSelectFailed");
            return_to_oper_list();
            return false;
        }
        sleep(500);
    }
    report_status("ParadoxReady");
    return true;
}

void asst::ParadoxRecognitionTask::return_to_oper_list() const
{
    if (!ProcessTask(*this, { "ParadoxReturnOperListFlag" }).set_retry_times(0).run()) {
        ProcessTask(*this, { "ParadoxReturnUntilOperList" }).set_retry_times(3).run();
    }
    ProcessTask(*this, { "BattleQuickFormationExpandRole" }).set_retry_times(3).run();
}

void asst::ParadoxRecognitionTask::report_status(const std::string& status)
{
    json::value info = basic_info_with_what(status);
    info["details"]["stage_name"] = Copilot.get_stage_name();
    info["details"]["operator"] = m_oper_name.name;
    callback(AsstMsg::SubTaskExtraInfo, info);
}

void asst::ParadoxRecognitionTask::add_file(int id, const std::string& navigate_name)
{
    m_paradox_files.emplace_back(id, navigate_name);
}

bool asst::ParadoxRecognitionTask::match_oper(const std::string& name) const
{
    return m_oper_name.name == name || m_oper_name.name_en == name || m_oper_name.name_jp == name ||
           m_oper_name.name_kr == name || m_oper_name.name_tw == name;
}
