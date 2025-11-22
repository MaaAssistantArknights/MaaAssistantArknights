#include "RoguelikeEncounterOptionAnalyzer.h"

#include "Config/TaskData.h"
#include "MaaUtils/ImageIo.h"
#include "Task/Roguelike/RoguelikeConfig.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Vision/RegionOCRer.h"

bool asst::RoguelikeEncounterOptionAnalyzer::analyze()
{
    LogTraceFunction;

    // ————————————————————————————————————————————————————————————————
    // validate m_theme
    // ————————————————————————————————————————————————————————————————
    if (!RoguelikeConfig::is_valid_theme(m_theme)) [[unlikely]] {
        Log.error(__FUNCTION__, std::format("| Invalid roguelike theme: {}; failed to analyze", m_theme));
        return false;
    }

    if (m_theme != RoguelikeTheme::JieGarden) [[unlikely]] {
        Log.error(__FUNCTION__, std::format("| Unsupported roguelike theme: {}; failed to analyze", m_theme));
        return false;
    }
    // ————————————————————————————————————————————————————————————————

    MultiMatcher::ResultsVecOpt option_analyze_ret = analyze_options(m_image);
    if (!option_analyze_ret) {
        Log.error(__FUNCTION__, "| Failed to recognise any options");
        save_img(m_image, "m_image");
        return false;
    }
#ifdef ASST_DEBUG
    save_img(m_image, "m_image");
#endif
    MultiMatcher::ResultsVec option_analyze_result = option_analyze_ret.value();

    const MatchTaskPtr enabled_task_ptr =
        Task.get<MatchTaskInfo>(m_theme + "@RoguelikeEncounterOptionAnalyzer-OptionHeaderBar-Enabled");
    const MatchTaskPtr templ_task_ptr = Task.get<MatchTaskInfo>(m_theme + "@RoguelikeEncounterOptionAnalyzer-Option");
    const OcrTaskPtr text_task_ptr =
        Task.get<OcrTaskInfo>(m_theme + "@RoguelikeEncounterOptionAnalyzer-OptionHeaderBar-Text");

    Result result;
    for (const auto& [rect, score, templ_name] : option_analyze_result) {
        Option option;

        Matcher enabled_analyzer(make_roi(m_image, rect));
        enabled_analyzer.set_task_info(enabled_task_ptr);
        option.enabled = enabled_analyzer.analyze().has_value();

        Rect templ_rect = templ_task_ptr->specific_rect;
        templ_rect.y = rect.y;
        option.templ = make_roi(m_image, templ_rect);

        RegionOCRer ocrer(option.enabled ? option.templ : binarize_for_ocr(option.templ));
        ocrer.set_task_info(text_task_ptr);
        if (ocrer.analyze()) {
            option.text = ocrer.get_result().text;
        }
        else {
            Log.error(__FUNCTION__, "Failed to recognise option text");
            save_img(option.templ, "option template");
        }

        Log.info(
            "RoguelikeEncounterOptionAnalyzer | Found",
            option.enabled ? "enabled" : "disabled",
            "option:",
            option.text);
        result.emplace_back(std::move(option));
    }

    m_result = std::move(result);

    return true;
}

std::optional<int> asst::RoguelikeEncounterOptionAnalyzer::merge_image(const cv::Mat& new_img)
{
    LogTraceFunction;

    // ————————————————————————————————————————————————————————————————
    // validate m_theme
    // ————————————————————————————————————————————————————————————————
    if (!RoguelikeConfig::is_valid_theme(m_theme)) {
        Log.error(__FUNCTION__, std::format("| Invalid roguelike theme: {}; failed to merge images", m_theme));
        return std::nullopt;
    }

    if (m_theme != RoguelikeTheme::JieGarden) {
        Log.error(__FUNCTION__, std::format("| Unsupported roguelike theme: {}; failed to merge images", m_theme));
        return std::nullopt;
    }

    // ————————————————————————————————————————————————————————————————
    // handle special cases
    // ————————————————————————————————————————————————————————————————
    const int last_option_y_in_new_img = get_last_option_y(new_img);
    if (last_option_y_in_new_img == UNDEFINED) [[unlikely]] {
        Log.error(__FUNCTION__, "| No option is recognised in new_img; failed to merge images");
        save_img(new_img, "new_img");
        return std::nullopt;
    }

    if (m_image.empty()) {
        Log.info(__FUNCTION__, "| m_image is empty; replace m_image with new_img");
        set_image(new_img);
        set_last_option_y(last_option_y_in_new_img);
        return new_img.rows;
    }

    if (new_img.cols != m_image.cols) [[unlikely]] {
        Log.error(
            __FUNCTION__,
            std::format(
                "| new_img width ({}) does not match m_image width ({}); failed to merge images",
                new_img.cols,
                m_image.cols));
        return std::nullopt;
    }

    // ————————————————————————————————————————————————————————————————
    // initialise m_last_option_y when needed
    // ————————————————————————————————————————————————————————————————
    if (m_last_option_y == UNDEFINED) {
        Log.info(__FUNCTION__, "| Initialising m_last_option_y...");
        const int last_option_y = get_last_option_y(m_image);
        if (last_option_y == UNDEFINED) {
            Log.warn(__FUNCTION__, "| No option is recognised in m_image; replace m_image with new_img");
            set_image(new_img);
            set_last_option_y(last_option_y_in_new_img);
            return new_img.rows;
        }
        set_last_option_y(last_option_y);
    }

    // ————————————————————————————————————————————————————————————————
    // calculate offset and rel_y
    // ————————————————————————————————————————————————————————————————
    Rect overlap_rect_in_m_image =
        Task.get<MatchTaskInfo>(m_theme + "@RoguelikeEncounterOptionAnalyzer-Option")->specific_rect;
    overlap_rect_in_m_image.y = m_last_option_y;
    cv::Mat overlap_option_templ = make_roi(m_image, overlap_rect_in_m_image);

    Matcher::ResultOpt overlap_match_ret = match_option(m_theme, new_img, overlap_option_templ);
    if (!overlap_match_ret) {
        Log.error(__FUNCTION__, "Overlap match failed; failed to merge images");
        save_img(m_image, "m_image");
        save_img(new_img, "new_img");
        save_img(overlap_option_templ, "overlap_option_templ");
        return std::nullopt;
    }
    const Rect& overlap_rect_in_new_img = overlap_match_ret.value().rect;

    const int offset = (new_img.rows - overlap_rect_in_new_img.y) - (m_image.rows - overlap_rect_in_m_image.y);
    if (offset <= 0) {
        Log.info("The offset", offset, "is less than or equal to zero; cancel the image merging");
        return offset;
    }
    const int rel_y = m_image.rows + offset - new_img.rows;

    // ————————————————————————————————————————————————————————————————
    // prepare overlay rects
    // ————————————————————————————————————————————————————————————————
    cv::Rect overlay_rect { overlap_rect_in_new_img.x,
                            overlap_rect_in_new_img.y,
                            new_img.cols - overlap_rect_in_new_img.x,
                            new_img.rows - overlap_rect_in_new_img.y };
    cv::Rect overlay_rect_in_merged_image = overlay_rect;
    overlay_rect_in_merged_image.y = m_last_option_y;

    // ————————————————————————————————————————————————————————————————
    // merge images
    // ————————————————————————————————————————————————————————————————
    cv::Mat merged_image = cv::Mat { m_image.rows + offset, m_image.cols, m_image.type(), cv::Scalar(0) };
    m_image.copyTo(merged_image(cv::Rect { 0, 0, m_image.cols, m_image.rows }));
    new_img(overlay_rect).copyTo(merged_image(overlay_rect_in_merged_image));

    set_image(merged_image);
    set_last_option_y(last_option_y_in_new_img + rel_y);

    return offset;
}

void asst::RoguelikeEncounterOptionAnalyzer::set_theme(const std::string& theme)
{
    // ————————————————————————————————————————————————————————————————
    // validate theme
    // ————————————————————————————————————————————————————————————————
    if (!RoguelikeConfig::is_valid_theme(theme)) {
        Log.error(
            __FUNCTION__,
            std::format("| Invalid roguelike theme: {}; failed to set theme; reverting to {}", theme, m_theme));
        return;
    }

    if (theme != RoguelikeTheme::JieGarden) {
        Log.error(
            __FUNCTION__,
            std::format("| Unsupported roguelike theme: {}; failed to set theme; reverting to {}", theme, m_theme));
        return;
    }
    // ————————————————————————————————————————————————————————————————

    m_theme = theme;
    Log.info(__FUNCTION__, "| Set theme to", theme);
}

asst::Matcher::ResultOpt asst::RoguelikeEncounterOptionAnalyzer::match_option(
    const std::string& theme,
    const cv::Mat& image,
    const cv::Mat& option_templ)
{
    LogTraceFunction;

    // ————————————————————————————————————————————————————————————————
    // validate theme
    // ————————————————————————————————————————————————————————————————
    if (!RoguelikeConfig::is_valid_theme(theme)) [[unlikely]] {
        Log.error(__FUNCTION__, std::format("| Invalid roguelike theme: {}; failed to match option", theme));
        return std::nullopt;
    }

    if (theme != RoguelikeTheme::JieGarden) [[unlikely]] {
        Log.error(__FUNCTION__, std::format("| Unsupported roguelike theme: {}; failed to match option", theme));
        return std::nullopt;
    }
    // ————————————————————————————————————————————————————————————————

    const MatchTaskPtr option_match_task = Task.get<MatchTaskInfo>(theme + "@RoguelikeEncounterOptionAnalyzer-Option");

    Matcher option_matcher(image);
    option_matcher.set_task_info(option_match_task);
    option_matcher.set_templ(option_templ);

    Rect option_match_roi = option_match_task->roi;
    option_match_roi.height = image.rows - (WindowHeightDefault - option_match_roi.height);
    option_matcher.set_roi(option_match_roi);

    return option_matcher.analyze();
}

asst::MultiMatcher::ResultsVecOpt asst::RoguelikeEncounterOptionAnalyzer::analyze_options(const cv::Mat& image) const
{
    LogTraceFunction;

    const MatchTaskPtr option_analyze_task =
        Task.get<MatchTaskInfo>(m_theme + "@RoguelikeEncounterOptionAnalyzer-OptionHeaderBar");

    MultiMatcher option_analyzer(image);
    option_analyzer.set_task_info(option_analyze_task);

    Rect option_analyze_roi = option_analyze_task->roi;
    option_analyze_roi.height = image.rows - (WindowHeightDefault - option_analyze_roi.height);
    option_analyzer.set_roi(option_analyze_roi);

    if (!option_analyzer.analyze()) {
        return std::nullopt;
    }

    MultiMatcher::ResultsVec option_analyze_result = option_analyzer.get_result();
    sort_by_horizontal_(option_analyze_result); // 按照垂直方向排序（从上到下）

    return option_analyze_result;
}

int asst::RoguelikeEncounterOptionAnalyzer::get_last_option_y(const cv::Mat& image) const
{
    LogTraceFunction;

    const MultiMatcher::ResultsVecOpt option_analyze_ret = analyze_options(image);
    if (!option_analyze_ret) {
        Log.error("get_last_option_y | Fail to recognise any option");
        save_img(image);
        return UNDEFINED;
    }
    return option_analyze_ret.value().back().rect.y;
}

void asst::RoguelikeEncounterOptionAnalyzer::set_last_option_y(int last_option_y)
{
    m_last_option_y = last_option_y;
    Log.info("RoguelikeEncounterOptionAnalyzer | m_last_option y set to", last_option_y);
}

cv::Mat asst::RoguelikeEncounterOptionAnalyzer::binarize_for_ocr(const cv::Mat& image)
{
    if (image.empty()) [[unlikely]] {
        return {};
    }

    cv::Mat image_gray;
    cv::cvtColor(image, image_gray, cv::COLOR_BGR2GRAY);

    cv::Mat thresh;
    cv::threshold(image_gray, thresh, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    cv::Mat thresh_bgr;
    cv::cvtColor(thresh, thresh_bgr, cv::COLOR_GRAY2BGR);

    return thresh_bgr;
}

bool asst::RoguelikeEncounterOptionAnalyzer::save_img(const cv::Mat& image, const std::string_view description)
{
    return utils::save_debug_image(image, utils::path("debug") / "roguelike" / "encounter", true, description);
}
