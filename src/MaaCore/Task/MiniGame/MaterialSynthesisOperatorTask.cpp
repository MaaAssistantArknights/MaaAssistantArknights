#include "MaterialSynthesisOperatorTask.h"

#include <algorithm>
#include <ranges>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Hasher.h"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"
#include "Vision/Matcher.h"

namespace
{
constexpr int MaxOperatorPages = 100;
constexpr int MaxCacheRebuilds = 1;
}

asst::MaterialSynthesisOperatorTask::MaterialSynthesisOperatorTask(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain) :
    InfrastProductionTask(callback, inst, task_chain)
{
    set_mood_threshold(1.0);
}

bool asst::MaterialSynthesisOperatorTask::select_operator(const std::string& material_id, int material_level)
{
    LogTraceFunction;

    if (need_exit()) {
        return false;
    }
    close_quick_formation_expand_role();
    for (int attempt = 0; attempt <= MaxCacheRebuilds; ++attempt) {
        if (need_exit()) {
            return false;
        }

        if (!m_cache_valid) {
            if (!rebuild_cache()) {
                if (need_exit()) {
                    return false;
                }
                Log.warn("MaterialSynthesis | operator cache scan failed", attempt);
                invalidate_cache();
                if (!is_operator_list()) {
                    return false;
                }
                swipe_to_the_left_of_operlist();
                continue;
            }
        }
        else {
            Log.info("MaterialSynthesis | operator cache hit", m_operator_cache.size());
        }

        const auto best_index = find_best_operator(material_id, material_level);
        if (!best_index) {
            Log.warn("MaterialSynthesis | no cached operator is available", material_id, material_level);
            return false;
        }
        const infrast::Oper target = m_operator_cache.at(*best_index);

        swipe_to_the_left_of_operlist();
        if (need_exit()) {
            return false;
        }
        if (!click_clear_button() || !locate_and_select(target)) {
            if (need_exit()) {
                return false;
            }
            Log.warn("MaterialSynthesis | cached operator selection failed, rebuild cache", attempt);
            invalidate_cache();
            if (!is_operator_list()) {
                return false;
            }
            swipe_to_the_left_of_operlist();
            continue;
        }
        if (!confirm_selection()) {
            if (need_exit()) {
                return false;
            }
            Log.warn("MaterialSynthesis | operator confirmation failed", attempt);
            invalidate_cache();
            if (!is_operator_list()) {
                return false;
            }
            swipe_to_the_left_of_operlist();
            continue;
        }

        m_operator_cache.erase(m_operator_cache.begin() + *best_index);
        Log.info(
            "MaterialSynthesis | operator selected from cache",
            material_id,
            material_level,
            "remaining",
            m_operator_cache.size());
        return true;
    }

    Log.error("MaterialSynthesis | operator selection failed after cache rebuild");
    return false;
}

bool asst::MaterialSynthesisOperatorTask::rebuild_cache()
{
    Log.info("MaterialSynthesis | full operator scan started");
    m_operator_cache.clear();
    swipe_to_the_left_of_operlist();
    if (need_exit()) {
        return false;
    }

    std::vector<infrast::Oper> seen_operators;
    bool reached_end = false;
    int unchanged_pages = 0;
    for (int page = 0; page < MaxOperatorPages && !need_exit(); ++page) {
        const auto new_operators = scan_current_page(seen_operators);
        if (!new_operators) {
            invalidate_cache();
            return false;
        }
        Log.trace(
            "MaterialSynthesis | operator cache page",
            page,
            "new operators",
            *new_operators,
            "candidates",
            m_operator_cache.size());
        if (page != 0 && *new_operators == 0) {
            if (++unchanged_pages >= 2) {
                reached_end = true;
                break;
            }
        }
        else {
            unchanged_pages = 0;
        }
        if (need_exit()) {
            invalidate_cache();
            return false;
        }
        swipe_of_operlist();
    }
    if (need_exit()) {
        invalidate_cache();
        return false;
    }
    swipe_to_the_left_of_operlist();
    if (!reached_end) {
        Log.error("MaterialSynthesis | operator scan exceeded page limit");
        invalidate_cache();
        return false;
    }

    m_cache_valid = true;
    Log.info("MaterialSynthesis | full operator scan completed", m_operator_cache.size());
    return true;
}

std::optional<size_t> asst::MaterialSynthesisOperatorTask::scan_current_page(std::vector<infrast::Oper>& seen_operators)
{
    if (need_exit()) {
        return std::nullopt;
    }
    const auto image = ctrler()->get_image();
    InfrastOperImageAnalyzer analyzer(image);
    analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::All);
    analyzer.set_facility(facility_name());
    if (!analyzer.analyze()) {
        return std::nullopt;
    }
    analyzer.sort_by_loc();

    const int face_hash_threshold = Task.get("InfrastOperFace")->special_params[0];
    size_t new_operators = 0;
    for (const auto& oper : analyzer.get_result()) {
        if (oper.face_hash.empty()) {
            Log.warn("MaterialSynthesis | operator face hash is empty");
            return std::nullopt;
        }

        const bool seen = std::ranges::any_of(seen_operators, [&](const infrast::Oper& seen_oper) {
            return same_cached_operator(seen_oper, oper, face_hash_threshold);
        });
        if (!seen) {
            infrast::Oper fingerprint;
            fingerprint.face_hash = oper.face_hash;
            fingerprint.skills = oper.skills;
            fingerprint.operator_ids = oper.operator_ids;
            fingerprint.operator_id = oper.operator_id;
            seen_operators.emplace_back(std::move(fingerprint));
            ++new_operators;
        }
        if (oper.mood_ratio < m_mood_threshold) {
            continue;
        }

        const auto duplicate = std::ranges::find_if(m_operator_cache, [&](const infrast::Oper& cached) {
            return same_cached_operator(cached, oper, face_hash_threshold);
        });
        if (duplicate != m_operator_cache.cend()) {
            continue;
        }

        // 加工站始终需要一个兜底候选，因此保留未识别到技能的满心情干员。
        m_operator_cache.emplace_back(oper);
    }
    return new_operators;
}

std::optional<size_t>
    asst::MaterialSynthesisOperatorTask::find_best_operator(const std::string& material_id, int material_level) const
{
    if (m_operator_cache.empty()) {
        return std::nullopt;
    }

    size_t best_index = 0;
    double best_score = processing_score(m_operator_cache.front(), material_id, material_level);
    for (size_t index = 1; index < m_operator_cache.size(); ++index) {
        const double score = processing_score(m_operator_cache.at(index), material_id, material_level);
        // 只在严格更高时替换，保证同分候选保持首次扫描顺序。
        if (score > best_score) {
            best_index = index;
            best_score = score;
        }
    }

    Log.info("MaterialSynthesis | cached operator score", best_score, material_id, material_level);
    return best_index;
}

bool asst::MaterialSynthesisOperatorTask::locate_and_select(const infrast::Oper& target)
{
    const int face_hash_threshold = Task.get("InfrastOperFace")->special_params[0];
    std::vector<std::string> seen_face_hashes;
    int unchanged_pages = 0;
    for (int page = 0; page < MaxOperatorPages && !need_exit(); ++page) {
        const auto image = ctrler()->get_image();
        InfrastOperImageAnalyzer analyzer(image);
        analyzer.set_to_be_calced(
            InfrastOperImageAnalyzer::ToBeCalced::Mood | InfrastOperImageAnalyzer::ToBeCalced::FaceHash |
            InfrastOperImageAnalyzer::ToBeCalced::Selected);
        if (!analyzer.analyze()) {
            return false;
        }
        analyzer.sort_by_loc();

        size_t new_faces = 0;
        for (const auto& oper : analyzer.get_result()) {
            if (oper.face_hash.empty()) {
                return false;
            }
            const bool seen = std::ranges::any_of(seen_face_hashes, [&](const std::string& face_hash) {
                return Hasher::hamming(face_hash, oper.face_hash) < face_hash_threshold;
            });
            if (!seen) {
                seen_face_hashes.emplace_back(oper.face_hash);
                ++new_faces;
            }

            if (!avatar_matches(oper, target, face_hash_threshold)) {
                continue;
            }
            if (oper.selected || oper.mood_ratio < m_mood_threshold) {
                return false;
            }
            Log.info("MaterialSynthesis | cached operator located", page);
            if (need_exit()) {
                return false;
            }
            ctrler()->click(oper.rect);
            sleep(500);
            return review_selection(target);
        }

        if (page != 0 && new_faces == 0) {
            if (++unchanged_pages >= 2) {
                break;
            }
        }
        else {
            unchanged_pages = 0;
        }
        if (need_exit()) {
            return false;
        }
        swipe_of_operlist();
    }
    return false;
}

bool asst::MaterialSynthesisOperatorTask::review_selection(const infrast::Oper& target)
{
    if (need_exit()) {
        return false;
    }

    // 复用基建列表的排序方式，将已选干员稳定移动到第一页后再复核数量和头像。
    ProcessTask(*this, { "InfrastOperListTabSkillUnClicked" }).set_retry_times(0).run();
    if (need_exit()) {
        return false;
    }
    ProcessTask(*this, { "InfrastOperListTabWorkStatusUnClicked" }).set_retry_times(0).run();
    if (need_exit()) {
        return false;
    }
    swipe_to_the_left_of_operlist();
    if (need_exit()) {
        return false;
    }

    InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
    analyzer.set_to_be_calced(
        InfrastOperImageAnalyzer::ToBeCalced::FaceHash | InfrastOperImageAnalyzer::ToBeCalced::Selected);
    if (!analyzer.analyze()) {
        return false;
    }

    const int face_hash_threshold = Task.get("InfrastOperFace")->special_params[0];
    size_t selected_count = 0;
    bool target_selected = false;
    for (const auto& oper : analyzer.get_result()) {
        if (!oper.selected) {
            continue;
        }
        ++selected_count;
        target_selected = target_selected || avatar_matches(oper, target, face_hash_threshold);
    }

    Log.info("MaterialSynthesis | operator selection review", selected_count, target_selected);
    return selected_count == 1 && target_selected;
}

bool asst::MaterialSynthesisOperatorTask::confirm_selection()
{
    if (need_exit() || !click_confirm_button() || need_exit()) {
        return false;
    }
    ProcessTask verify(*this, { "MiniGame@MaterialSynthesis@Workshop" });
    verify.set_retry_times(2);
    return verify.run();
}

bool asst::MaterialSynthesisOperatorTask::is_operator_list() const
{
    if (need_exit()) {
        return false;
    }

    const cv::Mat image = ctrler()->get_image();
    Matcher analyzer(image);
    analyzer.set_task_info("BattleQuickFormationExpandRole");
    if (analyzer.analyze()) {
        return true;
    }
    analyzer.set_task_info("InfrastCloseQuickFormationExpandRole");
    return analyzer.analyze().has_value();
}

bool asst::MaterialSynthesisOperatorTask::same_cached_operator(
    const infrast::Oper& lhs,
    const infrast::Oper& rhs,
    int face_hash_threshold)
{
    if (lhs.skills != rhs.skills || !avatar_matches(lhs, rhs, face_hash_threshold)) {
        return false;
    }
    if (!lhs.operator_id.empty() && !rhs.operator_id.empty() && lhs.operator_id != rhs.operator_id) {
        return false;
    }
    if (!lhs.operator_ids.empty() && !rhs.operator_ids.empty() &&
        std::ranges::none_of(lhs.operator_ids, [&](const std::string& id) { return rhs.operator_ids.contains(id); })) {
        return false;
    }
    return true;
}

bool asst::MaterialSynthesisOperatorTask::avatar_matches(
    const infrast::Oper& lhs,
    const infrast::Oper& rhs,
    int face_hash_threshold)
{
    if (lhs.face_hash.empty() || rhs.face_hash.empty()) {
        return false;
    }
    return Hasher::hamming(lhs.face_hash, rhs.face_hash) < face_hash_threshold;
}

void asst::MaterialSynthesisOperatorTask::invalidate_cache()
{
    m_cache_valid = false;
    m_operator_cache.clear();
    discard_pending_selection();
    Log.info("MaterialSynthesis | operator cache invalidated");
}

double asst::MaterialSynthesisOperatorTask::processing_score(
    const infrast::Oper& oper,
    const std::string& material_id,
    int material_level)
{
    double score = 0;
    for (const auto& skill : oper.skills) {
        const auto& id = skill.id;
        if (id == "bskill_ws_asc1" && material_id.size() == 4 && material_id.starts_with("32")) {
            score += 0.7;
        }
        else if (id == "bskill_ws_asc2" && material_id.size() == 4 && material_id.starts_with("32")) {
            score += 0.8;
        }
        else if (id == "bskill_hire_kalts2" || id == "bskill_ws_p_kalts2") {
            score += 0.8;
        }
        else if (id == "bskill_ws_p5") {
            continue;
        }
        else if (id == "bskill_ws_p4") {
            score += 0.65;
        }
        else if (id == "bskill_ws_p3") {
            score += 0.6;
        }
        else if (id == "bskill_ws_evolve4") {
            score += 1.0;
        }
        else if (id == "bskill_ws_evolve3") {
            score += 0.8;
        }
        else if (id == "bskill_ws_evolve2") {
            score += 0.75;
        }
        else if (id == "bskill_ws_evolve1") {
            score += 0.7;
        }
        else if (id == "bskill_ws_free") {
            score += 0.8 - material_level * 0.1;
        }
        else if (id == "bskill_ws_cost_blemishine") {
            score += 0.4;
        }
        else if (id == "bskill_ws_bonus1" && material_level < 4) {
            score += 0.9;
        }
        else if (id == "bskill_ws_bonus2" && material_level == 4) {
            score += 0.9;
        }
        else if (id == "bskill_ws_alloyblock" && material_id == "31024") {
            score += 1.0;
        }
        else if (id == "bskill_ws_orirock" && (material_id == "30014" || material_id == "30013")) {
            score += 0.9;
        }
        else if (id == "bskill_ws_device" && (material_id == "30064" || material_id == "30063")) {
            score += 0.9;
        }
        else if (id == "bskill_ws_crystalline" && (material_id == "31034" || material_id == "30145")) {
            score += 0.8;
        }
        else if (id == "bskill_ws_skill3" && (material_id == "3302" || material_id == "3303")) {
            score += 1.8;
        }
        else if (id == "bskill_ws_skill2" && (material_id == "3302" || material_id == "3303")) {
            score += 1.75;
        }
        else if (id == "bskill_ws_skill1" && (material_id == "3302" || material_id == "3303")) {
            score += 1.7;
        }
    }
    return score;
}
