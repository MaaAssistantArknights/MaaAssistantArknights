#include "ParadoxAutoTask.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Controller/Controller.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/Miscellaneous/ParadoxListTask.h"
#include "Task/Miscellaneous/ParadoxRecognitionTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::ParadoxAutoTask::set_files(const std::vector<std::pair<int, std::string>>& files)
{
    m_candidates.clear();
    for (const auto& [id, filename] : files) {
        const auto content = json::open(utils::path(filename));
        if (!content) {
            return false;
        }
        const auto stage = content->get("stage_name", std::string());
        if (!stage.starts_with("mem_") || !stage.ends_with("_1") || stage.size() <= 6) {
            LogError << "Invalid paradox stage:" << stage;
            return false;
        }
        const auto suffix = "_" + stage.substr(4, stage.size() - 7);
        std::string name;
        for (const auto& [char_id, oper] : BattleData.get_all_chars()) {
            if (oper && char_id.starts_with("char_") && char_id.ends_with(suffix) &&
                oper->role != battle::Role::Drone) {
                if (!name.empty()) {
                    LogError << "Ambiguous paradox stage:" << stage;
                    return false;
                }
                name = oper->name;
            }
        }
        if (name.empty()) {
            LogWarn << "Unknown paradox stage:" << stage;
            continue;
        }
        m_candidates[name].push_back({ id, filename });
    }
    return !m_candidates.empty();
}

void asst::ParadoxAutoTask::report(const std::string& what, const std::string& name, int id)
{
    auto info = basic_info_with_what(what);
    info["details"]["operator"] = name;
    info["details"]["copilot_id"] = id;
    callback(AsstMsg::SubTaskExtraInfo, info);
}

bool asst::ParadoxAutoTask::_run()
{
    LogTraceFunction;
    const auto& client = ctrler()->get_client_type();
    if (!client.empty() && client != "Official" && client != "Bilibili") {
        LogError << "Automatic paradox navigation requires Chinese client resources";
        return false;
    }

    bool all_succeeded = true;
    while (!m_candidates.empty() && !need_exit()) {
        std::vector<std::string> names;
        names.reserve(m_candidates.size());
        for (const auto& [name, _] : m_candidates) {
            names.emplace_back(name);
        }

        ParadoxListTask scan(m_callback, m_inst, m_task_chain);
        scan.set_task_id(m_task_id).set_retry_times(0);
        scan.set_next_only(true, names);
        if (!scan.run()) {
            report("ParadoxAutoRecognitionFailed", "");
            return false;
        }
        if (scan.get_result().empty()) {
            break;
        }

        const auto name = scan.get_result().front().name;
        auto candidates = std::move(m_candidates.at(name));
        m_candidates.erase(name);

        bool succeeded = false;
        bool from_detail = true;
        for (const auto& candidate : candidates) {
            if (need_exit()) {
                return false;
            }
            report("ParadoxAutoAttempt", name, candidate.id);
            const auto result = run_candidate(candidate, from_detail);
            from_detail = false;
            if (result == CandidateResult::Completed || result == CandidateResult::AlreadyCompleted) {
                succeeded = true;
                report("ParadoxAutoCompleted", name, candidate.id);
                break;
            }
            report("ParadoxAutoCandidateFailed", name, candidate.id);
        }
        if (!succeeded) {
            all_succeeded = false;
            report("ParadoxAutoStageFailed", name);
        }
    }
    return !need_exit() && all_succeeded;
}

asst::ParadoxAutoTask::CandidateResult
    asst::ParadoxAutoTask::run_candidate(const Candidate& candidate, bool from_detail)
{
    std::string paradox_status;
    bool battle_failed = false;
    bool battle_finished = false;
    AsstCallback on_event = [&](AsstMsg msg, const json::value& info, Assistant* inst) {
        if (msg == AsstMsg::SubTaskExtraInfo) {
            const auto what = info.get("what", std::string());
            if (what.starts_with("Paradox")) {
                paradox_status = what;
            }
        }
        if (msg == AsstMsg::SubTaskStart && info.get("subtask", std::string()) == "ProcessTask") {
            const auto task = info.get("details", "task", std::string());
            battle_failed = battle_failed || task.ends_with("FightMissionFailed-Bypass");
            battle_finished = battle_finished || task.ends_with("EndOfAction-Bypass");
        }
        m_callback(msg, info, inst);
    };

    auto battle = std::make_shared<BattleProcessTask>(on_event, m_inst, m_task_chain);
    battle->set_task_id(m_task_id).set_retry_times(0);
    battle->set_wait_until_end(true);

    ParadoxRecognitionTask navigation(on_event, m_inst, m_task_chain);
    navigation.set_task_id(m_task_id).set_retry_times(0);
    navigation.set_battle_task_ptr(battle);
    navigation.set_from_detail(from_detail);
    navigation.add_file(candidate.id, candidate.filename);
    if (!navigation.run()) {
        return paradox_status == "ParadoxAlreadyCompleted" ? CandidateResult::AlreadyCompleted
                                                           : CandidateResult::Failed;
    }

    ProcessTask start(on_event, m_inst, m_task_chain);
    start.set_task_id(m_task_id).set_retry_times(3);
    start.set_tasks({ "BattleStartAll" }).set_ignore_error(false);
    if (!start.run() || !battle->run() || need_exit()) {
        return CandidateResult::Failed;
    }

    ProcessTask settlement(on_event, m_inst, m_task_chain);
    settlement.set_task_id(m_task_id).set_retry_times(3);
    settlement.set_tasks({ "Copilot@WaitUntilEndOfAction-Bypass" });
    if (!settlement.run() || !battle_finished || battle_failed || need_exit()) {
        return CandidateResult::Failed;
    }
    return CandidateResult::Completed;
}
