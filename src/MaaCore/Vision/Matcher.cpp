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
        if (m_log_tracing && max_val > 0.5) { // 得分太低的肯定不对，没必要打印
            Log.trace("match_templ |", templ_name, "score:", max_val, "rect:", rect, "roi:", m_roi);
        }

        double threshold = m_params.templ_thres[i];
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
        cv::Mat image_for_match;
        cv::Mat templ_for_match;
        if (method == MatchMethod::CcoeffHSV || method == MatchMethod::HSVCount) {
            cv::cvtColor(image, image_for_match, cv::COLOR_BGR2HSV);
            cv::cvtColor(templ, templ_for_match, cv::COLOR_BGR2HSV);
        }
        else {
            cv::cvtColor(image, image_for_match, cv::COLOR_BGR2RGB);
            cv::cvtColor(templ, templ_for_match, cv::COLOR_BGR2RGB);
        }

        // 目前所有的匹配都是用 TM_CCOEFF_NORMED
        int match_algorithm = cv::TM_CCOEFF_NORMED;

        auto calc_mask = [&](const cv::Mat& templ_for_mask, bool with_close)->std::optional<cv::Mat> {
            cv::Mat templ_for_gray_mask;
            cv::cvtColor(templ_for_mask, templ_for_gray_mask, cv::COLOR_BGR2GRAY);

            // Union all masks, not intersection
            cv::Mat mask = cv::Mat::zeros(templ_for_gray_mask.size(), CV_8UC1);
            for (const auto& range : params.mask_range) {
                cv::Mat current_mask;
                if (range.first.size() == 1 && range.second.size() == 1) {
                    cv::inRange(templ_for_gray_mask, range.first[0], range.second[0], current_mask);
                }
                else if (range.first.size() == 3 && range.second.size() == 3) {
                    cv::inRange(templ_for_mask, range.first, range.second, current_mask);
                }
                else {
                    Log.error("Invalid mask range");
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

        if (params.mask_range.empty()) {
            cv::matchTemplate(image_for_match, templ_for_match, matched, match_algorithm);
        }
        else {
            auto mask_opt = calc_mask(
                params.mask_with_src ? image_for_match : templ_for_match,
                params.mask_with_close);
            if (!mask_opt) {
                return {};
            }
            cv::matchTemplate(image_for_match, templ_for_match, matched, match_algorithm, mask_opt.value());
        }

        if (method == MatchMethod::RGBCount || method == MatchMethod::HSVCount) {
            // 待匹配图像与模板中指定颜色的像素数量比值，越接近1越相似
            auto templ_active_opt = calc_mask(templ_for_match, false);
            auto image_active_opt = calc_mask(image_for_match, false);
            if (!image_active_opt || !templ_active_opt) [[unlikely]] {
                return {};
            }
            const auto& templ_active = templ_active_opt.value();
            const auto& image_active = image_active_opt.value();
            cv::Mat zero = cv::Mat::zeros(templ_active.size(), CV_8U);
            cv::threshold(image_active, image_active, 1, 1, cv::THRESH_BINARY);
            // 把 SQDIFF 当 count 用，计算 image_active 在 templ_active 形状内的像素数量
            cv::Mat tp, fp;
            int tp_fn = cv::countNonZero(templ_active);
            cv::matchTemplate(image_active, zero, tp, cv::TM_SQDIFF, templ_active);
            tp.convertTo(tp, CV_32S);
            cv::Mat templ_inactive;
            cv::bitwise_not(templ_active, templ_inactive);
            cv::matchTemplate(image_active, zero, fp, cv::TM_SQDIFF, templ_inactive);
            fp.convertTo(fp, CV_32S);
            // 数色结果为 f1_score
            cv::Mat count_result;
            cv::divide(tp, tp + fp + tp_fn, count_result, 1, CV_32F);
            cv::add(matched / 2, count_result, matched);
            // RGBCount 和 HSVCount 的结果是 数色 和 模板匹配 的综合结果
            // FIXME:
            // 1. 直接取算数平均值，之前考虑过点积，但 sqrt(0.8) > 0.89，默认阈值 0.8 可能偏高导致 FN
            // 2. 这里计算速度比较慢，之后可以考虑给 count 糊一个实现，不要用 SQDIFF

        }
        results.emplace_back(RawResult { .matched = matched, .templ = templ, .templ_name = templ_name });
    }
    return results;
}
