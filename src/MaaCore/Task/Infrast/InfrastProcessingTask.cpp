#include "InfrastProcessingTask.h"

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
constexpr int MaxMaterialSynthesisOperatorPages = 100;
constexpr int MaxMaterialSynthesisCacheRebuilds = 1;
constexpr double MaterialSynthesisMoodThreshold = 1.0;
}

bool asst::InfrastProcessingTask::_run()
{
    m_all_available_opers.clear();

    // 不是自定义的也换不了加工站
    if (!is_use_custom_opers()) {
        Log.info("skip this room");
        return true;
    }
    // 加工站，啥也造不了，随便写一个
    set_product("Placeholder");

    if (current_room_config().skip) {
        Log.info("skip this room");
        return true;
    }

    swipe_to_the_left_of_main_ui();
    if (!enter_facility()) {
        swipe_to_right_of_main_ui();
        if (!enter_facility()) {
            return false;
        }
    }

    click_bottom_left_tab();

    ProcessTask(*this, { "InfrastProcessingEnterOperList" }).run();

    close_quick_formation_expand_role();

    for (int i = 0; i <= OperSelectRetryTimes; ++i) {
        if (need_exit()) {
            return false;
        }
        if (is_use_custom_opers()) {
            bool name_select_ret = swipe_and_select_custom_opers();
            if (name_select_ret) {
                break;
            }
            else {
                swipe_to_the_left_of_operlist();
                continue;
            }
        }

        if (!opers_detect_with_swipe()) {
            return false;
        }
        break;
    }
    click_confirm_button();
    click_return_button();

    return true;
}

bool asst::InfrastProcessingTask::select_operator(const std::string& material_id, int material_level)
{
    LogTraceFunction;

    if (need_exit()) {
        return false;
    }
    close_quick_formation_expand_role();
    for (int attempt = 0; attempt <= MaxMaterialSynthesisCacheRebuilds; ++attempt) {
        if (need_exit()) {
            return false;
        }

        if (!m_material_synthesis_cache_valid) {
            if (!rebuild_material_synthesis_cache()) {
                if (need_exit()) {
                    return false;
                }
                Log.warn("MaterialSynthesis | operator cache scan failed", attempt);
                invalidate_material_synthesis_cache();
                if (!is_material_synthesis_operator_list()) {
                    return false;
                }
                swipe_to_the_left_of_operlist();
                continue;
            }
        }
        else {
            Log.info("MaterialSynthesis | operator cache hit", m_material_synthesis_operator_cache.size());
        }

        const auto best_index = find_best_material_synthesis_operator(material_id, material_level);
        if (!best_index) {
            Log.warn("MaterialSynthesis | no cached operator is available", material_id, material_level);
            return false;
        }
        const infrast::Oper target = m_material_synthesis_operator_cache.at(*best_index);

        swipe_to_the_left_of_operlist();
        if (need_exit()) {
            return false;
        }
        if (!clear_material_synthesis_selection() || !locate_and_select_material_synthesis_operator(target)) {
            if (need_exit()) {
                return false;
            }
            Log.warn("MaterialSynthesis | cached operator selection failed, rebuild cache", attempt);
            invalidate_material_synthesis_cache();
            if (!is_material_synthesis_operator_list()) {
                return false;
            }
            swipe_to_the_left_of_operlist();
            continue;
        }
        if (!confirm_material_synthesis_selection()) {
            if (need_exit()) {
                return false;
            }
            Log.warn("MaterialSynthesis | operator confirmation failed", attempt);
            invalidate_material_synthesis_cache();
            if (!is_material_synthesis_operator_list()) {
                return false;
            }
            swipe_to_the_left_of_operlist();
            continue;
        }

        m_material_synthesis_operator_cache.erase(m_material_synthesis_operator_cache.begin() + *best_index);
        Log.info(
            "MaterialSynthesis | operator selected from cache",
            material_id,
            material_level,
            "remaining",
            m_material_synthesis_operator_cache.size());
        return true;
    }

    Log.error("MaterialSynthesis | operator selection failed after cache rebuild");
    return false;
}

bool asst::InfrastProcessingTask::rebuild_material_synthesis_cache()
{
    Log.info("MaterialSynthesis | full operator scan started");
    m_material_synthesis_operator_cache.clear();
    swipe_to_the_left_of_operlist();
    if (need_exit()) {
        return false;
    }

    bool reached_end = false;
    for (int page = 0; page < MaxMaterialSynthesisOperatorPages && !need_exit(); ++page) {
        const auto scan_result = scan_material_synthesis_page();
        if (!scan_result) {
            invalidate_material_synthesis_cache();
            return false;
        }
        Log.trace(
            "MaterialSynthesis | operator cache page",
            page,
            "skilled operators",
            scan_result->skilled_operators,
            "new candidates",
            scan_result->new_candidates,
            "cached candidates",
            m_material_synthesis_operator_cache.size());
        // 与其他单人生产设施一致，进入首个没有本设施技能的页面即视为扫描完成。
        if (scan_result->skilled_operators == 0) {
            reached_end = true;
            break;
        }
        if (need_exit()) {
            invalidate_material_synthesis_cache();
            return false;
        }
        swipe_of_operlist();
    }
    if (need_exit()) {
        invalidate_material_synthesis_cache();
        return false;
    }
    swipe_to_the_left_of_operlist();
    if (!reached_end) {
        Log.error("MaterialSynthesis | operator scan exceeded page limit");
        invalidate_material_synthesis_cache();
        return false;
    }

    m_material_synthesis_cache_valid = true;
    Log.info("MaterialSynthesis | full operator scan completed", m_material_synthesis_operator_cache.size());
    return true;
}

std::optional<asst::InfrastProcessingTask::MaterialSynthesisScanResult>
    asst::InfrastProcessingTask::scan_material_synthesis_page()
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
    MaterialSynthesisScanResult result { static_cast<size_t>(analyzer.get_num_of_opers_with_skills()), 0 };
    for (const auto& oper : analyzer.get_result()) {
        if (oper.face_hash.empty()) {
            Log.warn("MaterialSynthesis | operator face hash is empty");
            return std::nullopt;
        }

        if (oper.mood_ratio < MaterialSynthesisMoodThreshold) {
            continue;
        }

        const auto duplicate =
            std::ranges::find_if(m_material_synthesis_operator_cache, [&](const infrast::Oper& cached) {
                return same_material_synthesis_operator(cached, oper, face_hash_threshold);
            });
        if (duplicate != m_material_synthesis_operator_cache.cend()) {
            continue;
        }

        m_material_synthesis_operator_cache.emplace_back(oper);
        ++result.new_candidates;
    }
    return result;
}

std::optional<size_t> asst::InfrastProcessingTask::find_best_material_synthesis_operator(
    const std::string& material_id,
    int material_level) const
{
    if (m_material_synthesis_operator_cache.empty()) {
        return std::nullopt;
    }

    std::vector<infrast::ScoreOper> score_opers;
    score_opers.reserve(m_material_synthesis_operator_cache.size());
    for (const auto& oper : m_material_synthesis_operator_cache) {
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
    context.mood_threshold = MaterialSynthesisMoodThreshold;
    const auto result = infrast::select_best_opers(score_opers, context);
    if (result.indices.empty()) {
        return std::nullopt;
    }

    Log.info("MaterialSynthesis | cached operator score", result.score, material_id, material_level);
    return result.indices.front();
}

bool asst::InfrastProcessingTask::clear_material_synthesis_selection()
{
    Matcher analyzer(ctrler()->get_image());
    analyzer.set_task_info("InfrastClearButton");
    if (!analyzer.analyze()) {
        Log.info("MaterialSynthesis | operator selection is already clear");
        return true;
    }

    Log.info("MaterialSynthesis | clear current operator selection");
    return click_clear_button();
}

bool asst::InfrastProcessingTask::locate_and_select_material_synthesis_operator(const infrast::Oper& target)
{
    const int face_hash_threshold = Task.get("InfrastOperFace")->special_params[0];
    std::vector<std::string> seen_face_hashes;
    int unchanged_pages = 0;
    for (int page = 0; page < MaxMaterialSynthesisOperatorPages && !need_exit(); ++page) {
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

            if (!material_synthesis_avatar_matches(oper, target, face_hash_threshold)) {
                continue;
            }
            if (oper.selected || oper.mood_ratio < MaterialSynthesisMoodThreshold) {
                return false;
            }
            Log.info("MaterialSynthesis | cached operator located", page);
            if (need_exit()) {
                return false;
            }
            ctrler()->click(oper.rect);
            sleep(500);
            return review_material_synthesis_selection(target);
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

bool asst::InfrastProcessingTask::review_material_synthesis_selection(const infrast::Oper& target)
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
        target_selected = target_selected || material_synthesis_avatar_matches(oper, target, face_hash_threshold);
    }

    Log.info("MaterialSynthesis | operator selection review", selected_count, target_selected);
    return selected_count == 1 && target_selected;
}

bool asst::InfrastProcessingTask::confirm_material_synthesis_selection()
{
    if (need_exit()) {
        return false;
    }

    // 仅执行页面确认，不提交常规基建任务的待确认干员或排班状态。
    if (!ProcessTask(*this, { "InfrastDormConfirmButton" }).run() || need_exit()) {
        return false;
    }
    ProcessTask verify(*this, { "MiniGame@MaterialSynthesis@Workshop" });
    verify.set_retry_times(2);
    return verify.run();
}

bool asst::InfrastProcessingTask::is_material_synthesis_operator_list() const
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

bool asst::InfrastProcessingTask::same_material_synthesis_operator(
    const infrast::Oper& lhs,
    const infrast::Oper& rhs,
    int face_hash_threshold)
{
    if (lhs.skills != rhs.skills || !material_synthesis_avatar_matches(lhs, rhs, face_hash_threshold)) {
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

bool asst::InfrastProcessingTask::material_synthesis_avatar_matches(
    const infrast::Oper& lhs,
    const infrast::Oper& rhs,
    int face_hash_threshold)
{
    if (lhs.face_hash.empty() || rhs.face_hash.empty()) {
        return false;
    }
    return Hasher::hamming(lhs.face_hash, rhs.face_hash) < face_hash_threshold;
}

void asst::InfrastProcessingTask::invalidate_material_synthesis_cache()
{
    m_material_synthesis_cache_valid = false;
    m_material_synthesis_operator_cache.clear();
    Log.info("MaterialSynthesis | operator cache invalidated");
}
