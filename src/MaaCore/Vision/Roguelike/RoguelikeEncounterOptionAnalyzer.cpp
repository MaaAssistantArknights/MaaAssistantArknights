#include "RoguelikeEncounterOptionAnalyzer.h"

#include "Config/TaskData.h"
#include "Task/Roguelike/RoguelikeConfig.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

bool asst::RoguelikeEncounterOptionAnalyzer::analyze()
{
    // ————————————————————————————————————————————————————————————————
    // validate m_theme
    // ————————————————————————————————————————————————————————————————
    if (!RoguelikeConfig::is_valid_theme(m_theme)) {
        Log.error(__FUNCTION__, std::format("| Invalid roguelike theme: {}; failed to analyze", m_theme));
        return false;
    }

    if (m_theme != RoguelikeTheme::JieGarden) {
        Log.error(__FUNCTION__, std::format("| Unsupported roguelike theme: {}; failed to analyze", m_theme));
        return false;
    }

    // ————————————————————————————————————————————————————————————————
    const MatchTaskPtr option_analyze_task =
        Task.get<MatchTaskInfo>(m_theme + "@RoguelikeEncounterOptionAnalyzer-OptionHeaderBar");
    const MatchTaskPtr option_enabled_analyze_task =
        Task.get<MatchTaskInfo>(m_theme + "@RoguelikeEncounterOptionAnalyzer-OptionHeaderBar-Enabled");
    const OcrTaskPtr option_text_analyze_task =
        Task.get<OcrTaskInfo>(m_theme + "@RoguelikeEncounterOptionAnalyzer-OptionHeaderBar-Text");

    MultiMatcher option_analyzer(m_image);
    option_analyzer.set_task_info(option_analyze_task);
    if (!option_analyzer.analyze()) {
        return false;
    }

    MultiMatcher::ResultsVec match_results = option_analyzer.get_result();
    sort_by_horizontal_(match_results); // 按照垂直方向从上到下排序各列节点

    Result result;
    for (const auto& [rect, score, templ_name] : match_results) {
        Option option;

        Matcher enabled_analyzer(make_roi(m_image, rect));
        enabled_analyzer.set_task_info(option_enabled_analyze_task);
        option.enabled = enabled_analyzer.analyze().has_value();

        Rect option_rect = rect;
        option_rect.width = option_analyze_task->special_params[0];
        option.templ = make_roi(m_image, option_rect);

        RegionOCRer ocrer(option.templ);
        ocrer.set_task_info(option_text_analyze_task);
        if (ocrer.analyze()) {
            option.text = ocrer.get_result().text;
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

void asst::RoguelikeEncounterOptionAnalyzer::set_theme(const std::string& theme)
{
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

    m_theme = theme;
    Log.info(__FUNCTION__, "| Set theme to", theme);
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
    // prepare const variables
    // ————————————————————————————————————————————————————————————————
    const MatchTaskPtr option_analyze_task =
        Task.get<MatchTaskInfo>(m_theme + "@RoguelikeEncounterOptionAnalyzer-OptionHeaderBar");

    auto get_last_option_y = [option_analyze_task](const cv::Mat& img) -> int {
        MultiMatcher option_analyzer(img);
        option_analyzer.set_task_info(option_analyze_task);
        if (!option_analyzer.analyze()) {
            Log.error("get_last_option_y | Fail to recognise any option");
            save_img(img);
            return 0; // y 是不会为 0 的，所以用 0 来表示异常返回值就足够了。
        }
        MultiMatcher::ResultsVec match_results = option_analyzer.get_result();
        sort_by_horizontal_(match_results); // 按照垂直方向从上到下排序各列节点
        return match_results.back().rect.y;
    };

    // ————————————————————————————————————————————————————————————————
    // handle special cases
    // ————————————————————————————————————————————————————————————————
    const int last_option_y_in_new_img = get_last_option_y(new_img);
    if (last_option_y_in_new_img == 0) {
        Log.error(__FUNCTION__, "| No option is recognised in new_img; failed to merge images");
        return std::nullopt;
    }

    if (m_image.empty()) {
        Log.info(__FUNCTION__, "| m_image is empty; replace m_image with new_img");
        m_image = new_img;
        set_last_option_y(last_option_y_in_new_img);
        return new_img.cols;
    }

    // ————————————————————————————————————————————————————————————————
    // initialise m_last_option_y when needed
    // ————————————————————————————————————————————————————————————————
    if (m_last_option_y == 0) {
        Log.info(__FUNCTION__, "| Initialising m_last_option_y...");
        const int last_option_y = get_last_option_y(m_image);
        if (last_option_y == 0) {
            Log.warn(__FUNCTION__, "| No option is recognised in m_image; replace m_image with new_img");
            m_image = new_img;
            set_last_option_y(last_option_y_in_new_img);
            return new_img.cols;
        }
        set_last_option_y(last_option_y);
    }

    // ————————————————————————————————————————————————————————————————
    // calculate offset and rel_y
    // ————————————————————————————————————————————————————————————————
    const cv::Rect overlap_templ_rect { option_analyze_task->roi.x,
                                        m_last_option_y,
                                        option_analyze_task->special_params[0],
                                        option_analyze_task->special_params[1] };
    cv::Rect overlap_roi = make_rect<cv::Rect>(option_analyze_task->roi);
    overlap_roi.width = option_analyze_task->special_params[0];

    Matcher overlap_matcher(make_roi(new_img, overlap_roi));
    overlap_matcher.set_templ(m_image(overlap_templ_rect));
    overlap_matcher.set_threshold(0.7);
    overlap_matcher.set_method(MatchMethod::Ccoeff);

    if (!overlap_matcher.analyze()) {
        Log.error(__FUNCTION__, "Overlap match failed; failed to merge images");
        save_img(m_image);
        save_img(new_img);
        return std::nullopt;
    }

    const int overlap_y_in_new_image = option_analyze_task->roi.y + overlap_matcher.get_result().rect.y;
    const int offset = (new_img.rows - overlap_y_in_new_image) - (m_image.rows - overlap_templ_rect.y);
    if (offset <= 0) {
        Log.info("The offset", offset, "is less than or equal to zero; cancel the image merging");
        return offset;
    }
    const int rel_y = m_image.rows + offset - new_img.rows;

    // ————————————————————————————————————————————————————————————————
    // prepare overlay rect
    // ————————————————————————————————————————————————————————————————
    cv::Rect overlay_rect { option_analyze_task->roi.x,
                            overlap_y_in_new_image,
                            WindowWidthDefault - option_analyze_task->roi.x,
                            WindowHeightDefault - overlap_y_in_new_image };
    cv::Rect overlay_rect_in_new_image = overlay_rect;
    overlay_rect_in_new_image.y = m_last_option_y;

    // ————————————————————————————————————————————————————————————————
    // merge images
    // ————————————————————————————————————————————————————————————————
    cv::Mat new_image = cv::Mat { m_image.rows + offset, m_image.cols, m_image.type(), cv::Scalar(0) };
    m_image.copyTo(new_image(cv::Rect { 0, 0, m_image.cols, m_image.rows }));
    new_img(overlay_rect).copyTo(new_image(overlay_rect_in_new_image));

    m_image = new_image;
    set_last_option_y(last_option_y_in_new_img + rel_y);

    return offset;
}

void asst::RoguelikeEncounterOptionAnalyzer::set_last_option_y(int last_option_y)
{
    m_last_option_y = last_option_y;
    Log.info("RoguelikeEncounterOptionAnalyzer | m_last_option y set to", last_option_y);
}

bool asst::RoguelikeEncounterOptionAnalyzer::save_img(const cv::Mat& image)
{
    const auto relative_dir = utils::path("debug") / utils::path("roguelikeEncounter");
    const auto relative_path = relative_dir / (std::format("{}_raw.png", utils::format_now_for_filename()));
    Log.trace("Save image", relative_path);
    return imwrite(relative_path, image);
}
