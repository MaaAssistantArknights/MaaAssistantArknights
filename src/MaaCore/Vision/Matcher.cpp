#include "Matcher.h"

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

using namespace asst;

Matcher::ResultOpt Matcher::analyze() const
{
    const auto match_results = preproc_and_match(make_roi(m_image, m_roi), m_params);

    for (size_t i = 0; i < match_results.size(); ++i) {
        const auto& [matched, templ, templ_name] = match_results[i];
        if (matched.empty()) {
            continue;
        }

        double min_val = 0.0, max_val = 0.0;
        cv::Point min_loc, max_loc;
        cv::minMaxLoc(matched, &min_val, &max_val, &min_loc, &max_loc);

        Rect rect(max_loc.x + m_roi.x, max_loc.y + m_roi.y, templ.cols, templ.rows);
        if (std::isnan(max_val) || std::isinf(max_val)) {
            max_val = 0;
        }

        double threshold = m_params.templ_thres[i];
        if (m_log_tracing && max_val > 0.5 && max_val > threshold - 0.2) { // 得分太低的肯定不对，没必要打印
            Log.trace("match_templ |", templ_name, "score:", max_val, "rect:", rect, "roi:", m_roi);
        }
#ifdef ASST_DEBUG
        else {
            Log.debug("match_templ |", templ_name, "score:", max_val, "rect:", rect, "roi:", m_roi);
        }
#endif
        if (max_val < threshold) {
            continue;
        }

        // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
        m_result.rect = rect;
        m_result.score = max_val;
        m_result.templ_name = templ_name;
        return m_result;
    }

    return std::nullopt;
}

std::vector<Matcher::RawResult> Matcher::preproc_and_match(const cv::Mat& image, const MatcherConfig::Params& params)
{
    std::vector<Matcher::RawResult> results;
    for (size_t i = 0; i != params.templs.size(); ++i) {
        const auto& ptempl = params.templs[i];
        auto method = MatchMethod::Ccoeff;
        if (params.methods.size() <= i) {
            Log.warn("methods is empty, use default method: Ccoeff");
        }
        else {
            method = params.methods[i];
        }

        if (method == MatchMethod::Invalid) {
            Log.error(__FUNCTION__, "| invalid method");
            return {};
        }

        cv::Mat templ;
        std::string templ_name;

        if (std::holds_alternative<std::string>(ptempl)) {
            templ_name = std::get<std::string>(ptempl);
            templ = TemplResource::get_instance().get_templ(templ_name);
        }
        else if (std::holds_alternative<cv::Mat>(ptempl)) {
            templ = std::get<cv::Mat>(ptempl);
        }
        else {
            Log.error("templ is none");
        }

        if (templ.empty()) {
            Log.error("templ is empty!", templ_name);
#ifdef ASST_DEBUG
            throw std::runtime_error("templ is empty: " + templ_name);
#else
            return {};
#endif
        }

        if (templ.cols > image.cols || templ.rows > image.rows) {
            Log.error("templ size is too large", templ_name, "image size:", image.cols, image.rows,
                      "templ size:", templ.cols, templ.rows);
            return {};
        }

        cv::Mat matched;
        cv::Mat image_match, image_count, image_gray;
        cv::Mat templ_match, templ_count, templ_gray;
        cv::cvtColor(image, image_match, cv::COLOR_BGR2RGB);
        cv::cvtColor(templ, templ_match, cv::COLOR_BGR2RGB);
        cv::cvtColor(image, image_gray, cv::COLOR_BGR2GRAY);
        cv::cvtColor(templ, templ_gray, cv::COLOR_BGR2GRAY);
        if (method == MatchMethod::HSVCount) {
            cv::cvtColor(image, image_count, cv::COLOR_BGR2HSV);
            cv::cvtColor(templ, templ_count, cv::COLOR_BGR2HSV);
        }
        else if (method == MatchMethod::RGBCount) {
            image_count = image_match;
            templ_count = templ_match;
        }

        // 目前所有的匹配都是用 TM_CCOEFF_NORMED
        int match_algorithm = cv::TM_CCOEFF_NORMED;

        auto calc_mask = [&templ_name](
                             const MatchTaskInfo::Ranges mask_ranges,
                             const cv::Mat& templ,
                             const cv::Mat& templ_gray,
                             bool with_close)
            -> std::optional<cv::Mat> {
            // Union all masks, not intersection
            cv::Mat mask = cv::Mat::zeros(templ_gray.size(), CV_8UC1);
            for (const auto& range : mask_ranges) {
                cv::Mat current_mask;
                if (std::holds_alternative<MatchTaskInfo::GrayRange>(range)) {
                    const auto& gray_range = std::get<MatchTaskInfo::GrayRange>(range);
                    cv::inRange(templ_gray, gray_range.first, gray_range.second, current_mask);
                }
                else if (std::holds_alternative<MatchTaskInfo::ColorRange>(range)) {
                    const auto& color_range = std::get<MatchTaskInfo::ColorRange>(range);
                    cv::inRange(templ, color_range.first, color_range.second, current_mask);
                }
                else {
                    Log.error("The task with template", templ_name, "holds invalid mask range");
                    return std::nullopt;
                }
                cv::bitwise_or(mask, current_mask, mask);
            }

            if (with_close) {
                cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
                cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
            }
            return mask;
        };

        if (params.mask_ranges.empty()) {
            cv::matchTemplate(image_match, templ_match, matched, match_algorithm);
        }
        else {
            // match 时使用的 mask_range 当作 RGB 的
            auto mask_opt = calc_mask(
                params.mask_ranges,
                params.mask_src ? image_match : templ_match,
                params.mask_src ? image_gray : templ_gray,
                params.mask_close);
            if (!mask_opt) {
                return {};
            }
            cv::matchTemplate(image_match, templ_match, matched, match_algorithm, mask_opt.value());
        }

        if (method == MatchMethod::RGBCount || method == MatchMethod::HSVCount) {
            auto templ_active_opt = calc_mask(params.color_scales, templ_count, templ_gray, params.color_close);
            auto image_active_opt = calc_mask(params.color_scales, image_count, image_gray, params.color_close);
            if (!image_active_opt || !templ_active_opt) [[unlikely]] {
                return {};
            }
            cv::Mat templ_active = std::move(templ_active_opt).value();
            cv::Mat image_active = std::move(image_active_opt).value();

            cv::threshold(templ_active, templ_active, 1, 1, cv::THRESH_BINARY);
            cv::threshold(image_active, image_active, 1, 1, cv::THRESH_BINARY);
            // 把 CCORR 当 count 用，计算 image_active 在 templ_active 形状内的像素数量
            cv::Mat tp, fp;
            int tp_fn = cv::countNonZero(templ_active);
            cv::matchTemplate(image_active, templ_active, tp, cv::TM_CCORR);
            tp.convertTo(tp, CV_32S);
            cv::Mat templ_inactive = 1 - templ_active;
            // TODO: 这里 TP+FP 是 image_active 的 count，可以消掉一个 matchtemplate
            cv::matchTemplate(image_active, templ_inactive, fp, cv::TM_CCORR);
            fp.convertTo(fp, CV_32S);
            cv::Mat count_result;
            cv::divide(2 * tp, tp + fp + tp_fn, count_result, 1, CV_32F); // 数色结果为 f1_score
            cv::multiply(matched, count_result, matched);                 // 最终结果是数色和模板匹配的点积
        }
        results.emplace_back(RawResult { .matched = matched, .templ = templ, .templ_name = templ_name });
    }
    return results;
}
