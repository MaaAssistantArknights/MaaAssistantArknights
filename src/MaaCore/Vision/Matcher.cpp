#include "Matcher.h"

#include "MaaUtils/NoWarningCV.hpp"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "MaaUtils/ImageIo.h"
#include "MaskedCcoeffMatcher.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

using namespace asst;

Matcher::ResultOpt Matcher::analyze() const
{
    if (m_roi.empty()) {
        return std::nullopt;
    }
    const auto match_results = preproc_and_match(make_roi(m_image, m_roi), m_params);

    for (size_t i = 0; i < match_results.size(); ++i) {
        const auto& [matched, templ, templ_name, path] = match_results[i];
        if (matched.empty()) {
            continue;
        }

        double min_val = 0.0, max_val = 0.0;
        cv::Point min_loc, max_loc;
        cv::Mat valid_mask;
        cv::inRange(matched, 0.0f, 1.0f + 1e-5f, valid_mask);
        cv::minMaxLoc(matched, &min_val, &max_val, &min_loc, &max_loc, valid_mask);

        Rect rect(max_loc.x + m_roi.x, max_loc.y + m_roi.y, templ.cols, templ.rows);

        double threshold = m_params.templ_thres[i];
        const char* path_tag = path == MatchPath::Optimized ? "optimized" : "opencv";
        const auto& method_i = m_params.methods.size() > i ? m_params.methods[i] : MatchMethod::Ccoeff;
        std::string tag = "[";
        tag += path_tag;
        if (method_i == MatchMethod::HSVCount) {
            tag += "|hsv";
        }
        else if (method_i == MatchMethod::RGBCount) {
            tag += "|rgb";
        }
        tag += "]";
        if (m_log_tracing && max_val > 0.5 && max_val > threshold - 0.2) { // 得分太低的肯定不对，没必要打印
            Log.trace("match_templ |", templ_name, tag, "score:", max_val, "rect:", rect, "roi:", m_roi);
#ifdef ASST_DEBUG
            if (!m_params.methods.empty() && m_params.methods[0] == MatchMethod::HSVCount) {
                const cv::Rect expanded_roi(
                    std::max(rect.x - 200, 0),
                    std::max(rect.y - 50, 0),
                    std::min(rect.width + 400, m_image.cols - std::max(rect.x - 200, 0)),
                    std::min(rect.height + 100, m_image.rows - std::max(rect.y - 50, 0)));
                cv::Mat cropped = m_image(expanded_roi).clone();
                const cv::Rect roi_in_cropped(
                    rect.x - expanded_roi.x,
                    rect.y - expanded_roi.y,
                    rect.width,
                    rect.height);
                cv::rectangle(cropped, roi_in_cropped, cv::Scalar(0, 0, 255), 1);
                const std::string name = std::filesystem::path(templ_name).stem().string();
                const std::string text = name + " " + std::to_string(max_val);
                const cv::Size text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, nullptr);
                const cv::Point text_pos(
                    std::max(roi_in_cropped.x + roi_in_cropped.width / 2 - text_size.width / 2, 0),
                    std::max(roi_in_cropped.y - 5, text_size.height));
                cv::putText(cropped, text, text_pos, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);

                const static std::vector<int> jpeg_params = { cv::IMWRITE_JPEG_QUALITY,
                                                              95,
                                                              cv::IMWRITE_JPEG_OPTIMIZE,
                                                              1 };
                utils::save_debug_image(cropped, utils::path("debug") / "hsv", true, text, "", "jpeg", jpeg_params);
            }
#endif
        }
        else {
            Log.debug("match_templ |", templ_name, tag, "score:", max_val, "rect:", rect, "roi:", m_roi);
        }
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

    // Image-side color conversions: compute once, reuse across all templates
    cv::Mat image_match;
    cv::cvtColor(image, image_match, cv::COLOR_BGR2RGB);

    cv::Mat image_gray;
    if (!params.mask_ranges.empty() || !params.color_scales.empty()) {
        cv::cvtColor(image, image_gray, cv::COLOR_BGR2GRAY);
    }

    cv::Mat image_hsv;

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
            if (templ_name == "empty.png") {
                LogError << __FUNCTION__ << "| template is empty.png";
            }
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
            Log.error(
                "templ size is too large",
                templ_name,
                "image size:",
                image.cols,
                image.rows,
                "templ size:",
                templ.cols,
                templ.rows);
            return {};
        }

        cv::Mat matched;
        auto match_path = MatchPath::OpenCV;
        cv::Mat templ_match, templ_count, templ_gray;
        cv::cvtColor(templ, templ_match, cv::COLOR_BGR2RGB);
        if (!image_gray.empty()) {
            cv::cvtColor(templ, templ_gray, cv::COLOR_BGR2GRAY);
        }

        cv::Mat image_count;
        if (method == MatchMethod::HSVCount) {
            if (image_hsv.empty()) {
                cv::cvtColor(image, image_hsv, cv::COLOR_BGR2HSV);
            }
            image_count = image_hsv;
            cv::cvtColor(templ, templ_count, cv::COLOR_BGR2HSV);
        }
        else if (method == MatchMethod::RGBCount) {
            image_count = image_match;
            templ_count = templ_match;
        }

        // 目前所有的匹配都是用 TM_CCOEFF_NORMED
        int match_algorithm = cv::TM_CCOEFF_NORMED;

        auto calc_mask = [&templ_name](
                             const MatchTaskInfo::Ranges& mask_ranges,
                             const cv::Mat& templ,
                             const cv::Mat& templ_gray,
                             bool with_close) -> std::optional<cv::Mat> {
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
            // mask_src=false 时 mask 完全由模板决定，用 FFT 路径替代标量滑窗
            if (!params.mask_src) {
                const int mask_pixels = cv::countNonZero(mask_opt.value());
                if (mask_pixels == mask_opt.value().rows * mask_opt.value().cols) {
                    cv::matchTemplate(image_match, templ_match, matched, match_algorithm);
                }
                else if (MaskedCcoeffMatcher::should_fallback_to_opencv(
                             mask_pixels,
                             (image_match.rows - templ_match.rows + 1) * (image_match.cols - templ_match.cols + 1))) {
                    // matched 保持 empty，统一落到下面的 OpenCV masked matchTemplate
                }
                else {
                    auto& masked_ccoeff_matcher = MaskedCcoeffMatcher::get_instance();
                    const uint64_t templ_revision = TemplResource::get_instance().revision();
                    masked_ccoeff_matcher.sync_cache_revision(templ_revision);

                    // cache key：templ_name + mask_ranges
                    // 资源模板绑定 revision；cv::Mat 模板使用 row-wise 内容 hash
                    std::string fft_key = templ_name.empty()
                                              ? MaskedCcoeffMatcher::make_mat_cache_key(templ)
                                              : "res:" + std::to_string(templ_revision) + ":" + templ_name;
                    for (const auto& r : params.mask_ranges) {
                        if (std::holds_alternative<MatchTaskInfo::GrayRange>(r)) {
                            const auto& g = std::get<MatchTaskInfo::GrayRange>(r);
                            fft_key += ":G" + std::to_string(g.first) + '_' + std::to_string(g.second);
                        }
                        else if (std::holds_alternative<MatchTaskInfo::ColorRange>(r)) {
                            const auto& col = std::get<MatchTaskInfo::ColorRange>(r);
                            fft_key += ":C";
                            for (auto v : col.first) {
                                fft_key += std::to_string(v) + ',';
                            }
                            fft_key += '_';
                            for (auto v : col.second) {
                                fft_key += std::to_string(v) + ',';
                            }
                        }
                    }
                    fft_key += params.mask_close ? ":1" : ":0";

                    matched =
                        masked_ccoeff_matcher.match(image_match, templ_match, mask_opt.value(), fft_key, mask_pixels);
                    if (!matched.empty()) {
                        match_path = MatchPath::Optimized;
                    }
                }
            }
            if (matched.empty()) {
                cv::matchTemplate(image_match, templ_match, matched, match_algorithm, mask_opt.value());
                match_path = MatchPath::OpenCV;
            }
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
            // tp = image_active 与 templ_active 的共激活像素数（TM_CCORR 当 count 用）
            cv::Mat tp;
            int tp_fn = cv::countNonZero(templ_active);
            cv::matchTemplate(image_active, templ_active, tp, cv::TM_CCORR);
            tp.convertTo(tp, CV_32S);
            // sum_active = 每个窗口内 image_active 的总激活数
            // 由于 tp + fp = sum_active，用积分图代替第二次 matchTemplate
            cv::Mat image_active_f;
            image_active.convertTo(image_active_f, CV_32F);
            cv::Mat integ;
            cv::integral(image_active_f, integ, CV_32F);
            const int kh = templ_active.rows, kw = templ_active.cols;
            // sum_active[y,x] = integ[y+kh,x+kw] - integ[y,x+kw] - integ[y+kh,x] + integ[y,x]
            cv::Mat sum_active = integ(cv::Rect(kw, kh, tp.cols, tp.rows)) - integ(cv::Rect(0, kh, tp.cols, tp.rows)) -
                                 integ(cv::Rect(kw, 0, tp.cols, tp.rows)) + integ(cv::Rect(0, 0, tp.cols, tp.rows));
            cv::Mat sum_active_i;
            sum_active.convertTo(sum_active_i, CV_32S);
            cv::Mat count_result;
            cv::divide(2 * tp, sum_active_i + tp_fn, count_result, 1, CV_32F); // 数色结果为 f1_score

            if (params.pure_color) {
                matched = 1.0f;
            }

            cv::multiply(matched, count_result, matched); // 最终结果是数色和模板匹配的点积
        }
        results.emplace_back(
            RawResult { .matched = matched, .templ = templ, .templ_name = templ_name, .path = match_path });
    }
    return results;
}
