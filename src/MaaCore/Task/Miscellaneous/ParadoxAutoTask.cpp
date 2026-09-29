#include "ParadoxAutoTask.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Controller/Controller.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/Miscellaneous/ParadoxListTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/ParadoxDetailAnalyzer.h"

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
        const auto suffix = "_" + stage.substr(4, stage.size() - 6);
        std::string name;
        std::string operator_id;
        for (const auto& [char_id, oper] : BattleData.get_all_chars()) {
            // Exact suffix boundary excludes both summons and similarly named operators.
            if (oper && char_id.starts_with("char_") && char_id.ends_with(suffix) &&
                oper->role != battle::Role::Drone) {
                if (!name.empty()) {
                    LogError << "Ambiguous paradox stage:" << stage;
                    return false;
                }
                name = oper->name;
                operator_id = char_id;
            }
        }
        if (name.empty()) {
            // The client may have newer stages than this resource bundle.
            LogWarn << "Unknown paradox stage:" << stage;
            continue;
        }
        m_candidates[name].push_back({ id, filename, stage, operator_id });
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
        std::unordered_set<std::string> names;
        for (const auto& [name, _] : m_candidates) {
            names.insert(name);
        }
        ParadoxListTask scan(m_callback, m_inst, m_task_chain);
        scan.set_task_id(m_task_id).set_retry_times(0);
        scan.set_candidates(std::move(names));
        if (!scan.run()) {
            report("ParadoxAutoRecognitionFailed", "");
            return false;
        }
        const auto name = scan.get_result();
        if (name.empty()) {
            break;
        }
        auto candidates = std::move(m_candidates.at(name));
        // A failed operator must not be selected again during this run.
        m_candidates.erase(name);
        bool succeeded = false;
        for (const auto& candidate : candidates) {
            if (need_exit()) {
                return false;
            }
            ParadoxDetailAnalyzer detail(ctrler()->get_image());
            const auto actual_name = detail.analyze();
            if (!actual_name || *actual_name != name) {
                report("ParadoxAutoRecognitionFailed", name);
                return false;
            }
            if (ProcessTask(*this, { "ParadoxAlreadyCompleted" }).set_retry_times(0).run()) {
                succeeded = true;
                break;
            }
            report("ParadoxAutoAttempt", name, candidate.id);
            const bool battle_succeeded = run_candidate(candidate);
            if (need_exit()) {
                return false;
            }
            // Verify the game's completion status even if the battle task returned success.
            if (!ProcessTask(*this, { "ParadoxAutoReturnToList" }).set_retry_times(3).run()) {
                report("ParadoxAutoRecognitionFailed", name);
                return false;
            }
            ParadoxListTask retry(m_callback, m_inst, m_task_chain);
            retry.set_task_id(m_task_id).set_retry_times(0);
            retry.set_candidates({ name });
            retry.set_include_completed(true);
            if (!retry.run() || retry.get_result() != name) {
                report("ParadoxAutoRecognitionFailed", name);
                return false;
            }
            if (retry.completed()) {
                succeeded = true;
                report("ParadoxAutoCompleted", name, candidate.id);
                break;
            }
            LogInfo << "Paradox candidate did not complete stage:" << candidate.id
                    << "battle result:" << battle_succeeded;
            report("ParadoxAutoCandidateFailed", name, candidate.id);
        }
        if (!succeeded) {
            all_succeeded = false;
            report("ParadoxAutoStageFailed", name);
        }
    }
    return !need_exit() && all_succeeded;
}

bool asst::ParadoxAutoTask::run_candidate(const Candidate& candidate)
{
    if (!Copilot.load(utils::path(candidate.filename)) || Copilot.get_stage_name() != candidate.stage) {
        return false;
    }
    const auto oper = BattleData.find_oper_by_id(candidate.operator_id);
    if (!oper) {
        return false;
    }
    int skill = 1;
    for (const auto& group : Copilot.get_data().groups) {
        for (const auto& usage : group.opers) {
            if (usage.name == oper->name || usage.name == oper->name_en || usage.name == oper->name_jp ||
                usage.name == oper->name_kr || usage.name == oper->name_tw) {
                if (usage.skill >= 1 && usage.skill <= 3) {
                    skill = usage.skill;
                }
            }
        }
    }
    BattleProcessTask battle(m_callback, m_inst, m_task_chain);
    battle.set_task_id(m_task_id).set_retry_times(0);
    battle.set_wait_until_end(true);
    if (!battle.set_stage_name(candidate.stage) ||
        !ProcessTask(*this, { "ParadoxAutoPrepare" }).set_retry_times(3).run()) {
        return false;
    }
    if (oper->rarity > 2 &&
        !ProcessTask(*this, { "ParadoxChooseSkill" + std::to_string(skill) }).set_retry_times(3).run()) {
        return false;
    }
    if (!ProcessTask(*this, { "BattleStartAll" }).set_retry_times(3).run() || !battle.run() || need_exit()) {
        return false;
    }
    bool failed = false;
    bool finished = false;
    AsstCallback on_settlement = [&](AsstMsg msg, const json::value& info, Assistant* inst) {
        if (msg == AsstMsg::SubTaskStart && info.get("subtask", std::string()) == "ProcessTask") {
            const auto task = info.get("details", "task", std::string());
            failed = failed || task.ends_with("FightMissionFailed-Bypass");
            finished = finished || task.ends_with("EndOfAction-Bypass");
        }
        m_callback(msg, info, inst);
    };
    ProcessTask settlement(on_settlement, m_inst, m_task_chain);
    settlement.set_task_id(m_task_id).set_retry_times(3);
    settlement.set_tasks({ "Copilot@WaitUntilEndOfAction-Bypass" });
    return settlement.run() && finished && !failed && !need_exit();
}
