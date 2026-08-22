#include "MaterialSynthesisOperatorTask.h"

#include <algorithm>
#include <ranges>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/Infrast/InfrastScore.h"
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

    std::vector<infrast::ScoreOper> score_opers;
    score_opers.reserve(m_operator_cache.size());
    for (const auto& oper : m_operator_cache) {
        infrast::ScoreOper score_oper;
        for (const auto& skill : oper.skills) {
            score_oper.skills.emplace(skill.id);
        }
        score_oper.operator_id = oper.operator_id;
        score_oper.face_hash = oper.face_hash;
        score_oper.mood_ratio = oper.mood_ratio;
        score_opers.emplace_back(std::move(score_oper));
    }

    infrast::ScoreContext context;
    context.facility = facility_name();
    context.product = material_id;
    context.level = material_level;
    context.slots = 1;
    context.mood_threshold = m_mood_threshold;
    const auto result = infrast::select_best_opers(score_opers, context);
    if (result.indices.empty()) {
        return std::nullopt;
    }

    Log.info("MaterialSynthesis | cached operator score", result.score, material_id, material_level);
    return result.indices.front();
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
