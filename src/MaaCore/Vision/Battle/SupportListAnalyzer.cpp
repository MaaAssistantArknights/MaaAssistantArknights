#include "SupportListAnalyzer.h"

#include "Config/TaskData.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/DebugImageHelper.hpp"
#include "Vision/Miscellaneous/PixelAnalyzer.h"
#include "Vision/RegionOCRer.h"

bool asst::SupportListAnalyzer::analyze(const battle::Role role)
{
    LogTraceFunction;

    static constexpr std::string_view LOG_PREFIX = "SupportListAnalyzer";

    MultiMatcher::ResultsVecOpt support_unit_analyze_ret = analyze_support_units(m_image);
    if (!support_unit_analyze_ret) {
        Log.error(LOG_PREFIX, "| Failed to recognise any support unit");
        save_img(m_image, "m_image");
        return false;
    }
#ifdef ASST_DEBUG
    save_img(m_image, "m_image");
#endif
    MultiMatcher::ResultsVec support_unit_analyze_result = support_unit_analyze_ret.value();

#ifndef ASST_DEBUG
    bool need_save_img = false;
#endif

    const MatchTaskPtr flag_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-Flag");
    const OcrTaskPtr name_task_ptr = Task.get<OcrTaskInfo>("SupportListAnalyzer-NameOcr");
    const MatchTaskPtr elite_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-Elite");
    const OcrTaskPtr level_task_ptr = Task.get<OcrTaskInfo>("SupportListAnalyzer-LevelOcr");
    const MatchTaskPtr potential_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-Potential");
    const MatchTaskPtr module_status_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-ModuleDisabled");
    const MatchTaskPtr friendship_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-Friendship");
    const MatchTaskPtr templ_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-SupportUnit");

    std::vector<SupportUnit> results;
    for (const auto& [rect, score, templ_name] : support_unit_analyze_result) {
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

        RegionOCRer name_analyzer(m_image);
        name_analyzer.set_task_info(name_task_ptr);
        name_analyzer.set_roi(name_roi);
        if (!name_analyzer.analyze()) [[unlikely]] {
            Log.error(LOG_PREFIX, "| Failed to recognise the current support unit's name; skipping to the next one");
#ifndef ASST_DEBUG
            need_save_img = true;
#endif
            continue;
        }

        // canonical_oper_name 函数根据 role 对干员名进行消歧义，目前仅用于区分不同升变形态下的阿米娅
        const std::string name = canonical_oper_name(role, name_analyzer.get_result().text);

        // ————————————————————————————————————————————————————————————————
        // Elite
        // ————————————————————————————————————————————————————————————————
        const Rect elite_roi = rect.move(elite_task_ptr->rect_move);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(elite_roi), cv::Scalar(0, 255, 0), 2);
#endif

        Matcher elite_analyzer(m_image);
        elite_analyzer.set_task_info(elite_task_ptr);
        elite_analyzer.set_roi(elite_roi);
        int elite = 0;
        if (elite_analyzer.analyze()) {
            std::optional<int> ret = get_suffix_num(elite_analyzer.get_result().templ_name);
            if (!ret) [[unlikely]] {
                Log.error(LOG_PREFIX, "| Failed to analyze the current support unit's elite; skipping to the next one");
                continue;
            }
            elite = ret.value();
        }

        // ————————————————————————————————————————————————————————————————
        // Level
        // ————————————————————————————————————————————————————————————————
        const Rect level_roi = rect.move(level_task_ptr->rect_move);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(level_roi), cv::Scalar(0, 255, 0), 2);
#endif

        RegionOCRer level_analyzer(m_image);
        level_analyzer.set_task_info(level_task_ptr);
        level_analyzer.set_roi(level_roi);
        if (!level_analyzer.analyze()) [[unlikely]] {
            Log.error(LOG_PREFIX, "| Failed to recognise the current support unit's level; skipping to the next one");
#ifndef ASST_DEBUG
            need_save_img = true;
#endif
            continue;
        }
        int level = 0;
        if (!utils::chars_to_number(level_analyzer.get_result().text, level)) {
            Log.error(LOG_PREFIX, "| Failed to convert text", level_analyzer.get_result().text, "to number");
            Log.error(LOG_PREFIX, "| Failed to analyze the current support unit's level; skipping to the next one");
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

        MultiMatcher potential_analyzer(m_image);
        potential_analyzer.set_task_info(potential_task_ptr);
        potential_analyzer.set_roi(potential_roi);
        if (!potential_analyzer.analyze()) [[unlikely]] {
            Log.error(LOG_PREFIX, "| Failed to analyze the current support unit's potential; skipping to the next one");
#ifndef ASST_DEBUG
            need_save_img = true;
#endif
            continue;
        }
        std::optional<int> ret = get_suffix_num(potential_analyzer.get_result().front().templ_name);
        if (!ret) [[unlikely]] {
            Log.error(LOG_PREFIX, "| Failed to analyze the current support unit's potential; skipping to the next one");
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

        Matcher module_analyzer(m_image);
        module_analyzer.set_task_info(module_status_task_ptr);
        module_analyzer.set_roi(module_status_roi);
        const bool module_enabled = !module_analyzer.analyze();

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

            MatchTaskInfo::ColorRange color_range =
                std::get<MatchTaskInfo::ColorRange>(friendship_task_ptr->color_scales[0]);
            pixel_analyzer.set_lb(color_range.first);
            pixel_analyzer.set_ub(color_range.second);

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
        Rect templ_rect = templ_task_ptr->specific_rect;
        templ_rect.x = rect.x;

        SupportUnit support_unit { .templ = make_roi(m_image, templ_rect),
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

std::optional<int> asst::SupportListAnalyzer::merge_image(const cv::Mat& new_img)
{
    LogTraceFunction;

    // ————————————————————————————————————————————————————————————————
    // handle special cases
    // ————————————————————————————————————————————————————————————————
    const int last_support_unit_x_in_new_img = get_last_support_unit_x(new_img);
    if (last_support_unit_x_in_new_img == UNDEFINED) [[unlikely]] {
        Log.error(__FUNCTION__, "| No support unit is recognised in new_img; failed to merge images");
        save_img(new_img, "new_img");
        return std::nullopt;
    }

    if (m_image.empty()) {
        Log.info(__FUNCTION__, "| m_image is empty; replace m_image with new_img");
        set_image(new_img);
        set_last_support_unit_x(last_support_unit_x_in_new_img);
        return new_img.cols;
    }

    if (new_img.rows != m_image.rows) [[unlikely]] {
        Log.error(
            __FUNCTION__,
            std::format(
                "| new_img height ({}) does not match m_image height ({}); failed to merge images",
                new_img.rows,
                m_image.rows));
        return std::nullopt;
    }

    // ————————————————————————————————————————————————————————————————
    // initialise m_last_support_unit_x when needed
    // ————————————————————————————————————————————————————————————————
    if (m_last_support_unit_x == UNDEFINED) {
        Log.info(__FUNCTION__, "| Initialising m_last_support_unit_x...");
        const int last_support_unit_x = get_last_support_unit_x(m_image);
        if (last_support_unit_x == UNDEFINED) {
            Log.warn(__FUNCTION__, "| No support unit is recognised in m_image; replace m_image with new_img");
            set_image(new_img);
            set_last_support_unit_x(last_support_unit_x_in_new_img);
            return new_img.cols;
        }
        set_last_support_unit_x(last_support_unit_x);
    }

    // ————————————————————————————————————————————————————————————————
    // calculate offset and rel_x
    // ————————————————————————————————————————————————————————————————
    Rect overlap_rect_in_m_image = Task.get<MatchTaskInfo>("SupportListAnalyzer-SupportUnit")->specific_rect;
    overlap_rect_in_m_image.x = m_last_support_unit_x;
    cv::Mat overlap_option_templ = make_roi(m_image, overlap_rect_in_m_image);

    Matcher::ResultOpt overlap_match_ret = match_support_unit(new_img, overlap_option_templ);
    if (!overlap_match_ret) {
        Log.error(__FUNCTION__, "Overlap match failed; failed to merge images");
        save_img(m_image, "m_image");
        save_img(new_img, "new_img");
        save_img(overlap_option_templ, "overlap_option_templ");
        return std::nullopt;
    }
    const Rect& overlap_rect_in_new_img = overlap_match_ret.value().rect;

    const int offset = (new_img.cols - overlap_rect_in_new_img.x) - (m_image.cols - overlap_rect_in_m_image.x);
    if (offset <= 0) {
        Log.info("The offset", offset, "is less than or equal to zero; cancel the image merging");
        return offset;
    }
    const int rel_x = m_image.cols + offset - new_img.cols;

    // ————————————————————————————————————————————————————————————————
    // prepare overlay rects
    // ————————————————————————————————————————————————————————————————
    cv::Rect overlay_rect { overlap_rect_in_new_img.x,
                            overlap_rect_in_new_img.y,
                            new_img.cols - overlap_rect_in_new_img.x,
                            new_img.rows - overlap_rect_in_new_img.y };
    cv::Rect overlay_rect_in_merged_image = overlay_rect;
    overlay_rect_in_merged_image.x = m_last_support_unit_x;

    // ————————————————————————————————————————————————————————————————
    // merge images
    // ————————————————————————————————————————————————————————————————
    cv::Mat merged_image = cv::Mat { m_image.rows, m_image.cols + offset, m_image.type(), cv::Scalar(0) };
    m_image.copyTo(merged_image(cv::Rect { 0, 0, m_image.cols, m_image.rows }));
    new_img(overlay_rect).copyTo(merged_image(overlay_rect_in_merged_image));

    set_image(merged_image);
    set_last_support_unit_x(last_support_unit_x_in_new_img + rel_x);

    return offset;
}

asst::Matcher::ResultOpt
    asst::SupportListAnalyzer::match_support_unit(const cv::Mat& image, const cv::Mat& support_unit_templ)
{
    LogTraceFunction;

    const MatchTaskPtr support_unit_match_task = Task.get<MatchTaskInfo>("SupportListAnalyzer-SupportUnit");

    Matcher support_unit_matcher(image);
    support_unit_matcher.set_task_info(support_unit_match_task);
    support_unit_matcher.set_templ(support_unit_templ);

    Rect support_unit_match_roi = support_unit_match_task->roi;
    support_unit_match_roi.width = image.cols - (WindowWidthDefault - support_unit_match_roi.width);
    support_unit_matcher.set_roi(support_unit_match_roi);

    return support_unit_matcher.analyze();
}

asst::MultiMatcher::ResultsVecOpt asst::SupportListAnalyzer::analyze_support_units(const cv::Mat& image)
{
    LogTraceFunction;

    const MatchTaskPtr support_unit_analyze_task = Task.get<MatchTaskInfo>("SupportListAnalyzer-Flag");

    MultiMatcher support_unit_analyzer(image);
    support_unit_analyzer.set_task_info(support_unit_analyze_task);

    Rect support_unit_analyze_roi = support_unit_analyze_task->roi;
    support_unit_analyze_roi.width = image.cols - (WindowWidthDefault - support_unit_analyze_roi.width);
    support_unit_analyzer.set_roi(support_unit_analyze_roi);

    if (!support_unit_analyzer.analyze()) {
        return std::nullopt;
    }

    MultiMatcher::ResultsVec support_unit_analyze_result = support_unit_analyzer.get_result();
    sort_by_vertical_(support_unit_analyze_result); // 按照水平方向排序（从左到右）

    return support_unit_analyze_result;
}

int asst::SupportListAnalyzer::get_last_support_unit_x(const cv::Mat& image)
{
    LogTraceFunction;

    const MultiMatcher::ResultsVecOpt support_unit_analyze_ret = analyze_support_units(image);
    if (!support_unit_analyze_ret) {
        Log.error("get_last_support_unit_x | Fail to recognise any support unit");
        save_img(image);
        return UNDEFINED;
    }
    return support_unit_analyze_ret.value().back().rect.x;
}

void asst::SupportListAnalyzer::set_last_support_unit_x(const int last_support_unit_x)
{
    m_last_support_unit_x = last_support_unit_x;
    Log.info("SupportListAnalyzer | m_last_support_unit_x set to", last_support_unit_x);
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

bool asst::SupportListAnalyzer::save_img(const cv::Mat& image, const std::string_view description)
{
    return utils::save_debug_image(image, utils::path("debug") / "supportListAnalyzer", true, description);
}
