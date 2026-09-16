#include "InfrastMaterialCraftTask.h"

#include <algorithm>
#include <charconv>
#include <ranges>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/WorkingDir.hpp"
#include "Vision/Hasher.h"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"
#include "Vision/RegionOCRer.h"

using namespace asst;

namespace
{
constexpr int MaxOperatorPages = 100;
constexpr int MaxOperatorChanges = 100;
constexpr int MaxMood = 24;

std::unordered_set<std::string> skill_ids(const infrast::Oper& oper)
{
    std::unordered_set<std::string> result;
    for (const auto& skill : oper.skills) {
        result.emplace(skill.id);
    }
    return result;
}

bool contains_face(const std::vector<std::string>& faces, const std::string& face)
{
    if (face.empty()) {
        return false;
    }
    const int threshold = Task.get("InfrastOperFace")->special_params[0];
    return std::ranges::any_of(faces, [&](const auto& seen) { return Hasher::hamming(seen, face) < threshold; });
}
}

std::optional<int> InfrastMaterialCraftTask::read_processing_number(
    const cv::Mat& image,
    const std::string& task_name,
    bool fraction) const
{
    if (need_exit()) {
        return std::nullopt;
    }
    RegionOCRer analyzer(image);
    analyzer.set_task_info(task_name);
    const auto result = analyzer.analyze();
    if (!result || result->score < 0.9) {
        return std::nullopt;
    }
    std::string_view text = result->text;
    if (fraction) {
        if (!text.ends_with("/24")) {
            return std::nullopt;
        }
        text.remove_suffix(3);
    }
    int number = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), number);
    if (error != std::errc() || end != text.data() + text.size() || number < 0 || (fraction && number > MaxMood)) {
        return std::nullopt;
    }
    return number;
}

std::optional<int> InfrastMaterialCraftTask::read_processing_mood(const cv::Mat& image) const
{
    if (!is_craft_page(image)) {
        return std::nullopt;
    }
    if (match_workshop_template(image, "MaterialCraft-NoOperator")) {
        return 0;
    }
    return read_processing_number(image, "MaterialCraft-OperatorMood", true);
}

std::optional<int> InfrastMaterialCraftTask::prepare_processing_operator(const Formula& formula, int remaining)
{
    // Always use the game value: skills can change both the recipe cost and the remaining mood.
    std::vector<std::string> rejected_faces;
    for (int change = 0; change <= MaxOperatorChanges && !need_exit(); ++change) {
        const auto image = ctrler()->get_image();
        const auto mood = read_processing_mood(image);
        const auto quantity = read_craft_count();
        const auto cost = read_processing_number(image, "MaterialCraft-MoodCost");
        if (quantity && *quantity == 0) {
            processing_operator_failure("MaterialsUnavailable");
            return std::nullopt;
        }
        if (!mood || !quantity || *quantity != 1 || !cost) {
            processing_operator_failure("MoodRecognitionFailed");
            return std::nullopt;
        }
        const bool sufficient = *mood > 0 && *cost <= *mood;
        update_processing_operator_mood(*mood);
        const bool needs_scoring = m_scored_processing_item != formula.item_id || !m_processing_score;
        if (sufficient && !needs_scoring) {
            if (*cost != m_processing_score->mood_cost) {
                processing_operator_failure("SkillVerificationFailed");
                return std::nullopt;
            }
            const int limit = *cost == 0 ? remaining : std::min(remaining, *mood / *cost);
            Log.info("MaterialCraft | operator ready", formula.item_id, "mood", *mood, "cost", *cost, "batches", limit);
            return limit;
        }
        if (change == MaxOperatorChanges || !select_processing_operator(formula, rejected_faces, *mood)) {
            processing_operator_failure("OperatorSelectionFailed");
            return std::nullopt;
        }
        // Stationing preserves the selected recipe. Verify it without reopening the selector.
        if (!selected_formula_matches(formula)) {
            processing_operator_failure("FormulaVerificationFailed");
            return std::nullopt;
        }
    }
    return std::nullopt;
}

bool InfrastMaterialCraftTask::processing_mood_sufficient() const
{
    if (need_exit()) {
        return false;
    }
    const auto image = ctrler()->get_image();
    const auto mood = read_processing_mood(image);
    const auto cost = read_processing_number(image, "MaterialCraft-MoodCost");
    return mood && cost && *mood > 0 && *cost <= *mood;
}

void InfrastMaterialCraftTask::update_processing_operator_mood(int mood)
{
    if (m_processing_operator) {
        m_processing_candidates[*m_processing_operator].mood = mood;
    }
}

bool InfrastMaterialCraftTask::enter_processing_operator_list()
{
    if (need_exit() || !is_craft_page(ctrler()->get_image()) ||
        !ProcessTask(*this, { "InfrastProcessingEnterOperList" }).run() || need_exit()) {
        return false;
    }
    close_quick_formation_expand_role();
    if (need_exit() || !ProcessTask(*this, { "InfrastOperListTabSkillUnClicked", "Stop" }).run()) {
        return false;
    }
    swipe_to_the_left_of_operlist();
    return !need_exit();
}

bool InfrastMaterialCraftTask::scan_processing_operators()
{
    std::vector<ProcessingCandidate> candidates;
    std::vector<std::string> seen_faces;
    std::optional<bool> stainless_in_dorm;
    int unchanged_pages = 0;
    bool reached_end = false;
    for (int page = 0; page < MaxOperatorPages && !need_exit(); ++page) {
        InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
        analyzer.set_to_be_calced(InfrastOperImageAnalyzer::All);
        analyzer.set_facility("Processing");
        if (!analyzer.analyze()) {
            return false;
        }
        analyzer.sort_by_loc();
        size_t new_faces = 0;
        for (auto oper : analyzer.get_result()) {
            if (oper.face_hash.empty()) {
                return false;
            }
            if (contains_face(seen_faces, oper.face_hash)) {
                continue;
            }
            seen_faces.emplace_back(oper.face_hash);
            ++new_faces;
            if (oper.skills.empty()) {
                continue;
            }
            if (oper.operator_id.empty()) {
                resolve_operator_identity(oper);
            }
            if (oper.operator_id == "char_4072_ironmn" && oper.doing != infrast::Doing::Invalid) {
                stainless_in_dorm = oper.doing == infrast::Doing::Resting;
            }
            // If identity remains ambiguous, do not accidentally select an explicitly excluded operator.
            const bool possibly_excluded =
                oper.operator_id.empty() &&
                std::ranges::any_of(oper.operator_ids, infrast::is_excluded_processing_operator);
            if (possibly_excluded || infrast::is_excluded_processing_operator(oper.operator_id) ||
                (!oper.selected && oper.doing == infrast::Doing::Working)) {
                continue;
            }
            // Cache all workshop skills, including those that only apply to later recipes.
            candidates.push_back({ std::move(oper), std::nullopt });
        }
        if (new_faces == 0) {
            ++unchanged_pages;
        }
        else {
            unchanged_pages = 0;
        }
        if (analyzer.get_num_of_opers_with_skills() == 0 || unchanged_pages >= 2) {
            reached_end = true;
            break;
        }
        if (!need_exit()) {
            swipe_of_operlist();
        }
    }
    if (!reached_end || need_exit()) {
        return false;
    }
    m_processing_candidates = std::move(candidates);
    m_stainless_in_dorm = stainless_in_dorm;
    m_processing_operator.reset();
    for (size_t index = 0; index < m_processing_candidates.size(); ++index) {
        if (m_processing_candidates[index].oper.selected) {
            m_processing_operator = index;
        }
    }
    m_processing_candidates_scanned = true;
    Log.info("MaterialCraft | processing operator cache populated", m_processing_candidates.size());
    return true;
}

bool InfrastMaterialCraftTask::select_processing_operator(
    const Formula& formula,
    std::vector<std::string>& rejected_faces,
    int current_mood,
    bool replace_current)
{
    m_scored_processing_item.clear();
    m_processing_score.reset();
    bool list_open = false;
    if (!m_processing_candidates_scanned) {
        if (!enter_processing_operator_list() || !scan_processing_operators()) {
            return false;
        }
        list_open = true;
    }
    update_processing_operator_mood(current_mood);
    if (replace_current && m_processing_operator) {
        rejected_faces.emplace_back(m_processing_candidates[*m_processing_operator].oper.face_hash);
    }

    std::optional<size_t> best;
    std::optional<infrast::ProcessingOperatorScore> best_score;
    for (size_t index = 0; index < m_processing_candidates.size(); ++index) {
        const auto& candidate = m_processing_candidates[index];
        const auto& oper = candidate.oper;
        if (need_exit()) {
            return false;
        }
        if (oper.mood_ratio <= 0 || contains_face(rejected_faces, oper.face_hash)) {
            continue;
        }
        const auto score =
            infrast::score_processing_operator(skill_ids(oper), oper.operator_id, formula, m_stainless_in_dorm);
        if (!score || (candidate.mood && (*candidate.mood <= 0 || *candidate.mood < score->mood_cost))) {
            continue;
        }
        Log.info(
            "MaterialCraft | processing candidate",
            oper.operator_id,
            oper.face_hash,
            "bonus",
            score->bonus_percent,
            "mood cost",
            score->mood_cost,
            "probability lower bound",
            score->probability_is_lower_bound);
        if (!best || score->better_than(*best_score) ||
            (!best_score->better_than(*score) && oper.selected && !m_processing_candidates[*best].oper.selected)) {
            best = index;
            best_score = score;
        }
    }
    if (!best) {
        return false;
    }
    const auto& target = m_processing_candidates[*best].oper;
    if (list_open || m_processing_operator != best) {
        if (!list_open && !enter_processing_operator_list()) {
            return false;
        }
        swipe_to_the_left_of_operlist();
        if (!locate_processing_operator(target) || !confirm_processing_operator()) {
            return false;
        }
    }
    for (size_t index = 0; index < m_processing_candidates.size(); ++index) {
        m_processing_candidates[index].oper.selected = index == *best;
    }
    m_processing_operator = best;
    rejected_faces.emplace_back(target.face_hash);
    m_scored_processing_item = formula.item_id;
    m_processing_score = best_score;
    Log.info(
        "MaterialCraft | processing operator selected",
        target.operator_id,
        "bonus",
        best_score->bonus_percent,
        "probability",
        best_score->byproduct_probability(),
        "mood cost",
        best_score->mood_cost);
    return true;
}

bool InfrastMaterialCraftTask::locate_processing_operator(const infrast::Oper& target)
{
    std::vector<std::string> seen_faces;
    int unchanged_pages = 0;
    for (int page = 0; page < MaxOperatorPages && !need_exit(); ++page) {
        InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
        analyzer.set_to_be_calced(InfrastOperImageAnalyzer::All);
        analyzer.set_facility("Processing");
        if (!analyzer.analyze()) {
            return false;
        }
        size_t new_faces = 0;
        for (const auto& oper : analyzer.get_result()) {
            if (!contains_face(seen_faces, oper.face_hash)) {
                seen_faces.emplace_back(oper.face_hash);
                ++new_faces;
            }
            if (!contains_face({ target.face_hash }, oper.face_hash)) {
                continue;
            }
            // Re-recognize skills after the rewind, rather than trusting stale screen coordinates.
            if (oper.skills != target.skills || oper.mood_ratio <= 0 ||
                (!oper.selected && oper.doing == infrast::Doing::Working)) {
                return false;
            }
            if (!oper.selected && (need_exit() || !ctrler()->click(oper.rect) ||
                                   !craft_sleep(Task.get("MaterialCraft-AnimationDelay")->post_delay))) {
                return false;
            }
            return !need_exit() && review_processing_operator(target);
        }
        unchanged_pages = new_faces == 0 ? unchanged_pages + 1 : 0;
        if (unchanged_pages >= 2) {
            return false;
        }
        if (!need_exit()) {
            swipe_of_operlist();
        }
    }
    return false;
}

bool InfrastMaterialCraftTask::confirm_processing_operator()
{
    if (need_exit() || !click_confirm_button()) {
        return false;
    }
    for (int poll = 0; poll < 8 && !need_exit(); ++poll) {
        if (is_craft_page(ctrler()->get_image())) {
            return true;
        }
        if (!craft_sleep(Task.get("MaterialCraft-RetryDelay")->post_delay)) {
            return false;
        }
    }
    return false;
}

bool InfrastMaterialCraftTask::review_processing_operator(const infrast::Oper& target) const
{
    if (need_exit()) {
        return false;
    }
    InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
    analyzer.set_to_be_calced(InfrastOperImageAnalyzer::FaceHash | InfrastOperImageAnalyzer::Selected);
    if (!analyzer.analyze()) {
        return false;
    }
    int selected_count = 0;
    bool target_selected = false;
    for (const auto& oper : analyzer.get_result()) {
        if (oper.selected) {
            ++selected_count;
            target_selected = contains_face({ target.face_hash }, oper.face_hash);
        }
    }
    return selected_count == 1 && target_selected;
}

void InfrastMaterialCraftTask::processing_operator_failure(const std::string& reason)
{
    if (need_exit()) {
        return;
    }
    Log.error("MaterialCraft | operator preparation failed", reason);
    save_img(utils::path("debug") / utils::path("material_craft") / utils::path("operator_failed"));
    auto info = basic_info_with_what("MaterialCraftOperatorFailed");
    info["details"]["reason"] = reason;
    callback(AsstMsg::SubTaskExtraInfo, info);
}
