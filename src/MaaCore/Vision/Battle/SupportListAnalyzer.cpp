#include "SupportListAnalyzer.h"

#include "Config/TaskData.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/PixelAnalyzer.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

bool asst::SupportListAnalyzer::analyze(const battle::Role role)
{
    LogTraceFunction;

    const auto& flag_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-Flag");

    MultiMatcher flag_analyzer(m_image);
    flag_analyzer.set_task_info(flag_task_ptr);
    if (!flag_analyzer.analyze()) {
        Log.error(__FUNCTION__, "| No support unit detected");
        save_img(utils::path("debug") / utils::path("SupportListAnalyzer"));
        return false;
    }
    MultiMatcher::ResultsVec flag_analyze_result = flag_analyzer.get_result();
    sort_by_vertical_(flag_analyze_result); // 按照水平方向排序（从左到右）

    const OcrTaskPtr name_task_ptr = Task.get<OcrTaskInfo>("SupportListAnalyzer-NameOcr");
    const MatchTaskPtr elite_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-Elite");
    const OcrTaskPtr level_task_ptr = Task.get<OcrTaskInfo>("SupportListAnalyzer-LevelOcr");
    const MatchTaskPtr potential_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-Potential");
    const MatchTaskPtr module_status_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-ModuleDisabled");
    const MatchTaskPtr friendship_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-Friendship");

    RegionOCRer ocr_analyzer(m_image);
    Matcher match_analyzer(m_image);
#ifndef ASST_DEBUG
    bool need_save_img = false;
#endif

    std::vector<SupportUnit> results;
    for (const auto& [rect, score, templ_name] : flag_analyze_result) {
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(rect), cv::Scalar(0, 255, 0), 2);
#endif

        // ————————————————————————————————————————————————————————————————
        // Name
        // ————————————————————————————————————————————————————————————————
        const Rect name_roi = rect.move(name_task_ptr->rect_move);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(name_roi), cv::Scalar(0, 255, 0), 2);
#endif

        ocr_analyzer.set_task_info(name_task_ptr);
        ocr_analyzer.set_roi(name_roi);
        if (!ocr_analyzer.analyze()) [[unlikely]] {
            Log.error(
                "SupportListAnalyzer | Failed to recognise the current support unit's name; skipping to the next one");
#ifndef ASST_DEBUG
            need_save_img = true;
#endif
            continue;
        }

        // canonical_oper_name 函数根据 role 对干员名进行消歧义，目前仅用于区分不同升变形态下的阿米娅
        const std::string name = canonical_oper_name(role, ocr_analyzer.get_result().text);

        // ————————————————————————————————————————————————————————————————
        // Elite
        // ————————————————————————————————————————————————————————————————
        const Rect elite_roi = rect.move(elite_task_ptr->rect_move);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(elite_roi), cv::Scalar(0, 255, 0), 2);
#endif

        match_analyzer.set_task_info(elite_task_ptr);
        match_analyzer.set_roi(elite_roi);
        if (!match_analyzer.analyze()) [[unlikely]] {
            Log.error(
                "SupportListAnalyzer | Failed to analyze the current support unit's elite; skipping to the next one");
#ifndef ASST_DEBUG
            need_save_img = true;
#endif
            continue;
        }
        std::optional<int> ret = get_suffix_num(match_analyzer.get_result().templ_name);
        if (!ret) [[unlikely]] {
            Log.error(
                "SupportListAnalyzer",
                "| Failed to analyze the current support unit's elite; skipping to the next one");
            continue;
        }
        const int elite = ret.value();

        // ————————————————————————————————————————————————————————————————
        // Level
        // ————————————————————————————————————————————————————————————————
        const Rect level_roi = rect.move(level_task_ptr->rect_move);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(level_roi), cv::Scalar(0, 255, 0), 2);
#endif

        ocr_analyzer.set_task_info(level_task_ptr);
        ocr_analyzer.set_roi(level_roi);
        if (!ocr_analyzer.analyze()) [[unlikely]] {
            Log.error(
                "SupportListAnalyzer | Failed to recognise the current support unit's level; skipping to the next one");
#ifndef ASST_DEBUG
            need_save_img = true;
#endif
            continue;
        }
        int level = 0;
        if (!utils::chars_to_number(ocr_analyzer.get_result().text, level)) {
            Log.error("SupportListAnalyzer | Failed to convert text", ocr_analyzer.get_result().text, "to number");
            Log.error(
                "SupportListAnalyzer | Failed to analyze the current support unit's level; skipping to the next one");
#ifndef ASST_DEBUG
            need_save_img = true;
#endif
            continue;
        }

        // ————————————————————————————————————————————————————————————————
        // Potential
        // ————————————————————————————————————————————————————————————————
        const Rect potential_roi = rect.move(potential_task_ptr->rect_move);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(potential_roi), cv::Scalar(0, 255, 0), 2);
#endif

        match_analyzer.set_task_info(potential_task_ptr);
        match_analyzer.set_roi(potential_roi);
        if (!match_analyzer.analyze()) {
            Log.error(
                "SupportListAnalyzer",
                "| Failed to analyze the current support unit's potential; skipping to the next one");
#ifndef ASST_DEBUG
            need_save_img = true;
#endif
            continue;
        }
        ret = get_suffix_num(match_analyzer.get_result().templ_name);
        if (!ret) [[unlikely]] {
            Log.error(
                "SupportListAnalyzer",
                "| Failed to analyze the current support unit's potential; skipping to the next one");
            continue;
        }
        const int potential = ret.value();

        // ————————————————————————————————————————————————————————————————
        // Module Status
        // ————————————————————————————————————————————————————————————————
        const Rect module_status_roi = rect.move(module_status_task_ptr->rect_move);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(module_status_roi), cv::Scalar(0, 255, 0), 2);
#endif

        match_analyzer.set_task_info(module_status_task_ptr);
        match_analyzer.set_roi(module_status_roi);
        const bool module_enabled = !match_analyzer.analyze();

        // ————————————————————————————————————————————————————————————————
        // Friendship
        // ————————————————————————————————————————————————————————————————
        Friendship friendship;

        if (templ_name == flag_task_ptr->templ_names[0]) {
            const Rect friendship_roi = rect.move(friendship_task_ptr->rect_move);
#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(friendship_roi), cv::Scalar(0, 255, 0), 2);
#endif

            PixelAnalyzer pixel_analyzer(m_image);
            pixel_analyzer.set_roi(friendship_roi);
            pixel_analyzer.set_filter(PixelAnalyzer::Filter::HSV);

            MatchTaskInfo::ColorRange color_range = std::get<MatchTaskInfo::ColorRange>(flag_task_ptr->color_scales[0]);
            pixel_analyzer.set_lb(color_range.first);
            pixel_analyzer.set_lb(color_range.second);

            if (pixel_analyzer.analyze()) {
                friendship = Friendship::BestFriend;
            }
            else {
                friendship = Friendship::Friend;
            }
        }
        else {
            friendship = Friendship::Stranger;
        }

        // ————————————————————————————————————————————————————————————————

        SupportUnit support_unit { .rect = rect,
                                   .name = name,
                                   .elite = elite,
                                   .level = level,
                                   .potential = potential,
                                   .module_enabled = module_enabled,
                                   .friendship = friendship };
        Log.info(
            std::format(
                "SupportListAnalyzer"
                " | Found support unit {} from {} with elite {}, level {}, potential {} and module {}",
                support_unit.name,
                support_unit.friendship == Friendship::BestFriend
                    ? "a best friend"
                    : (support_unit.friendship == Friendship::Friend ? "a friend" : "a stranger"),
                support_unit.elite,
                support_unit.level,
                support_unit.potential,
                support_unit.module_enabled ? "enabled" : "disabled"));
        results.emplace_back(std::move(support_unit));
    }

#ifdef ASST_DEBUG
    save_img(utils::path("debug") / utils::path("SupportListAnalyzer"));
#else
    if (need_save_img) {
        save_img(utils::path("debug") / utils::path("SupportListAnalyzer"));
    }
#endif

    m_result = std::move(results);

    return !m_result.empty();
}

std::optional<int> asst::SupportListAnalyzer::get_suffix_num(const std::string& s, const char delimiter)
{
    const size_t pos = s.rfind(delimiter);
    if (pos == std::string::npos) [[unlikely]] {
        Log.error(__FUNCTION__, "| Unsupported string", s);
        return std::nullopt;
    }

    std::string num_str = s.substr(pos + 1);
    if (num_str.ends_with(".png")) {
        num_str.erase(num_str.size() - 4);
    }

    int num = 0;
    if (!utils::chars_to_number(num_str, num)) [[unlikely]] {
        Log.error(__FUNCTION__, "| Failed to convert text", num_str, "to number");
        return std::nullopt;
    }

    return num;
}
