#include "DepotImageAnalyzer.h"

#include "MaaUtils/NoWarningCV.hpp"

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <numbers>
#include <string_view>

bool asst::DepotImageAnalyzer::analyze()
{
    LogTraceFunction;

    m_all_items_roi.clear();
    m_result.clear();
    m_quantity_recognition_complete = true;
    m_reached_last_item = false;
    m_unrecognized_item_rect.reset();

    if (m_cached_templs.empty()) {
        prepare_cached_templates();
    }
    // 因为模板素材的尺寸与实际截图中素材尺寸不符，所以这里先对原图进行一下缩放
    resize();
    bool ret = analyze_base_rect();
    if (!ret) {
        return false;
    }

    ret = analyze_all_items();

#ifdef ASST_DEBUG
    m_image_draw = m_image_draw_resized;
    save_img(utils::path("debug") / utils::path("depot"));
#endif
    return ret;
}

const std::vector<std::string>& asst::DepotImageAnalyzer::get_ordered_item_ids() const
{
    return m_item_ids.empty() ? ItemData.get_ordered_material_item_id() : m_item_ids;
}

void asst::DepotImageAnalyzer::prepare_cached_templates()
{
    LogTraceFunction;

    for (const auto& item_id : get_ordered_item_ids()) {
        cv::Mat templ = TemplResource::get_instance().get_templ(item_id).clone();
        m_template_mean_colors[item_id] = cv::mean(templ(get_center_rect(templ)));
        // 抹去右下角 80x50 区域（防止影响匹配）
        templ(cv::Rect { templ.cols - 80, templ.rows - 50, 80, 50 }) = cv::Scalar { 0, 0, 0 };
        m_cached_templs[item_id] = std::move(templ);
    }
}

// 计算 BGR 均值差
double asst::DepotImageAnalyzer::color_diff(const cv::Scalar& a, const cv::Scalar& b)
{
    const double d0 = a[0] - b[0];
    const double d1 = a[1] - b[1];
    const double d2 = a[2] - b[2];
    return d0 * d0 + d1 * d1 + d2 * d2;
}

cv::Rect asst::DepotImageAnalyzer::get_center_rect(const cv::Mat& img, int width, int height)
{
    width = std::min(width, img.cols);
    height = std::min(height, img.rows);
    int x = std::max(0, (img.cols - width) / 2);
    int y = std::max(0, (img.rows - height) / 2);
    return { x, y, width, height };
}

// 用平均颜色筛选候选模板
std::vector<std::string> asst::DepotImageAnalyzer::filter_candidates_by_color(
    const cv::Mat& roi,
    const std::unordered_map<std::string, cv::Scalar>& template_mean_colors,
    double max_diff) // 色差阈值，可调
{
    std::vector<std::string> result;

    auto mat = roi(get_center_rect(roi));
    cv::Scalar roi_mean = cv::mean(mat);
    for (const auto& [id, templ_mean] : template_mean_colors) {
        if (color_diff(roi_mean, templ_mean) <= max_diff) {
            result.push_back(id);
        }
    }

    return result;
}

void asst::DepotImageAnalyzer::set_match_begin_pos(size_t pos) noexcept
{
    m_match_begin_pos = pos;
}

size_t asst::DepotImageAnalyzer::get_match_begin_pos() const noexcept
{
    return m_match_begin_pos;
}

void asst::DepotImageAnalyzer::resize()
{
    LogTraceFunction;

    m_resized_rect = Task.get("DepotMatchData")->roi;
    cv::Size d_size(m_resized_rect.width, m_resized_rect.height);
    cv::resize(m_image, m_image_resized, d_size, 0, 0, cv::INTER_AREA);
#ifdef ASST_DEBUG
    cv::resize(m_image_draw, m_image_draw_resized, d_size, 0, 0, cv::INTER_AREA);
#endif
}

template <typename F>
cv::Mat asst::DepotImageAnalyzer::image_from_function(const cv::Size& size, const F& func)
{
    auto result = cv::Mat1f { size, CV_32F };
    for (int i = 0; i < result.cols; ++i) {
        for (int j = 0; j < result.rows; ++j) {
            result.at<float>(j, i) = func(i, j);
        }
    }
    return result;
}

bool asst::DepotImageAnalyzer::analyze_base_rect()
{
    LogTraceFunction;

    static constexpr int tm_size = 183 * 720 / 1080;

    auto task_ptr = Task.get<MatchTaskInfo>("DepotBaseRect");
    int x_period = task_ptr->roi.width * m_image_resized.cols / WindowWidthDefault;
    int y_first = task_ptr->roi.y * m_image_resized.rows / WindowHeightDefault;
    int y_period = task_ptr->roi.height * m_image_resized.rows / WindowHeightDefault;

    cv::Mat hist_x;
    cv::Mat resized_gray;
    cv::cvtColor(m_image_resized, resized_gray, cv::COLOR_RGB2GRAY);
    cv::reduce(resized_gray, hist_x, 0, cv::REDUCE_AVG, CV_32F);

    cv::Mat sine_image = image_from_function({ resized_gray.cols, 1 }, [=](int x, int) -> float {
        return static_cast<float>(std::sin(2. * std::numbers::pi_v<double> * x / x_period));
    });

    cv::Mat cosine_image = image_from_function({ resized_gray.cols, 1 }, [=](int x, int) -> float {
        return static_cast<float>(std::cos(2. * std::numbers::pi_v<double> * x / x_period));
    });

    cv::Mat hist_sine;
    cv::Mat hist_cosine;
    cv::multiply(hist_x, sine_image, hist_sine);
    cv::multiply(hist_x, cosine_image, hist_cosine);

    const double s_v = cv::sum(hist_sine)[0];
    const double c_v = cv::sum(hist_cosine)[0];

    const double phase = std::atan2(s_v, c_v);
    double x_first = phase / (2. * std::numbers::pi_v<double>)*x_period + x_period / 2.;
    if (phase < 0) {
        x_first += x_period;
    }

    for (int x = static_cast<int>(x_first); x <= m_image_resized.cols; x += x_period) {
        for (int y = y_first; y <= m_image_resized.rows; y += y_period) {
            Rect item_roi = Rect(x - tm_size / 2, y - tm_size / 2, tm_size, tm_size);
            if (!m_resized_rect.include(item_roi)) {
                continue;
            }
            m_all_items_roi.emplace_back(item_roi);
#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw_resized, make_rect<cv::Rect>(item_roi), cv::Scalar(0, 255, 0), 2);
#endif
        }
    }

    return !m_all_items_roi.empty();
}

bool asst::DepotImageAnalyzer::analyze_all_items()
{
    LogTraceFunction;

    for (const Rect& roi : m_all_items_roi) {
        ItemInfo info;
        size_t cur_pos = match_item(roi, info, m_match_begin_pos);
        if (cur_pos == NPos) {
            if (m_is_basic) {
                continue;
            }
            m_unrecognized_item_rect = resize_rect_to_raw_size(roi);
            break;
        }
        std::string item_id = info.item_id;

        if (!m_is_basic) {
            m_match_begin_pos = cur_pos + 1;
        }
        info.item_name = ItemData.get_item_name(item_id);
        const auto quantity = match_quantity(info);
        if (!quantity || (!m_is_basic && *quantity == 0)) {
            LogWarn << "Failed to recognize depot item quantity" << VAR(item_id) << VAR(info.item_name);
            m_quantity_recognition_complete = false;
            continue;
        }
        info.quantity = *quantity;
        if (!m_is_basic && m_match_begin_pos == get_ordered_item_ids().size()) {
            m_reached_last_item = true;
        }
#ifdef ASST_DEBUG
        cv::putText(
            m_image_draw_resized,
            item_id,
            cv::Point(roi.x, roi.y - 10),
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,
            cv::Scalar(0, 0, 255),
            2);
        cv::putText(
            m_image_draw_resized,
            std::to_string(info.quantity),
            cv::Point(roi.x, roi.y + 10),
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,
            cv::Scalar(0, 0, 255),
            2);
#endif
        info.rect = resize_rect_to_raw_size(info.rect);
        m_result.emplace(std::move(item_id), std::move(info));
    }
#ifdef ASST_DEBUG
    cv::Mat hsv;
    cv::cvtColor(m_image_resized, hsv, cv::COLOR_BGR2HSV);
#endif

    return !m_result.empty() || m_unrecognized_item_rect.has_value();
}

size_t asst::DepotImageAnalyzer::match_item(
    const Rect& roi,
    /* out */ ItemInfo& item_info,
    size_t begin_index,
    bool with_enlarge)
{
    LogTraceFunction;

    // spacing 有时候算的差一个像素，干脆把 roi 扩大一点好了
    Rect enlarged_roi = roi;
    if (with_enlarge) {
        enlarged_roi = Rect(roi.x - 20, roi.y - 5, roi.width + 40, roi.height + 10);
    }

    // 用颜色过滤得到候选模板 ID，传入缓存的模板均值和阈值
    const auto candidates =
        filter_candidates_by_color(m_image_resized(make_rect<cv::Rect>(roi)), m_template_mean_colors);
    Log.info("Candidate templates count:", candidates.size());

    Matcher analyzer(m_image_resized);
    analyzer.set_task_info("DepotMatchData");
    analyzer.set_roi(enlarged_roi);

    MatchRect matched;
    std::string matched_item_id;
    size_t matched_index = NPos;

    const auto& all_items = get_ordered_item_ids();

    // 按候选模板ID顺序过滤，且保证 index >= begin_index
    size_t extra_count = 0;
    for (const auto& item_id : candidates) {
        // 找到该 item_id 在 all_items 中的位置（保证顺序和 begin_index）
        auto it = std::find(
            all_items.begin() + static_cast<std::vector<std::string>::difference_type>(begin_index),
            all_items.end(),
            item_id);
        if (it == all_items.end()) {
            continue; // 不满足 begin_index 之后的条件，跳过
        }
        size_t index = std::distance(all_items.begin(), it);

        auto templ_it = m_cached_templs.find(item_id);
        if (templ_it == m_cached_templs.end()) {
            continue;
        }

        analyzer.set_templ(templ_it->second);
        if (!analyzer.analyze()) {
            continue;
        }

        if (const double score = analyzer.get_result().score; score >= matched.score) {
            matched = analyzer.get_result();
            matched_item_id = item_id;
            matched_index = index;
        }

        // 匹配到了任一结果后，再往后匹配几个。
        // 因为有些相邻的材料长得很像（同一种类的）
        if (matched_index != NPos && ++extra_count >= 8) { // MaxExtraMatch = 8
            break;
        }
    }

    Log.info("Item id:", matched_item_id);
    if (matched_item_id.empty()) {
        return NPos;
    }

    item_info.item_id = matched_item_id;
    item_info.rect = matched.rect;
    return matched_index;
}

std::optional<int> asst::DepotImageAnalyzer::match_quantity(const ItemInfo& item)
{
    LogTraceFunction;

    auto task_ptr = Task.get<MatchTaskInfo>("DepotQuantity");
    auto item_templ = TemplResource::get_instance().get_templ(item.item_id);
    const Rect image_rect { 0, 0, m_image_resized.cols, m_image_resized.rows };
    if (item.rect.width <= 0 || item.rect.height <= 0 || !image_rect.include(item.rect) ||
        item_templ.size() != cv::Size(item.rect.width, item.rect.height) ||
        item_templ.type() != m_image_resized.type()) {
        LogWarn << "Invalid depot quantity item region" << VAR(item.item_id) << VAR(item.rect);
        return std::nullopt;
    }
    auto item_image = m_image_resized(make_rect<cv::Rect>(item.rect));
    cv::Mat quotient;
    cv::divide(
        item_image + cv::Scalar { 1, 1, 1 }, // I've forgot why I should plus 1 here
        item_templ + cv::Scalar { 1, 1, 1 }, // plus 1 to avoid divide by zero
        quotient,
        255);

    cv::Mat mask_r;
    cv::Mat mask_g;
    cv::Mat mask_b;
    static constexpr int lb = 60;
    static constexpr int ub = 140;
    cv::inRange(quotient, cv::Scalar { lb, 0, 0 }, cv::Scalar { ub, 255, 255 }, mask_r);
    cv::inRange(quotient, cv::Scalar { 0, lb, 0 }, cv::Scalar { 255, ub, 255 }, mask_g);
    cv::inRange(quotient, cv::Scalar { 0, 0, lb }, cv::Scalar { 255, 255, ub }, mask_b);
    cv::Mat mask;
    cv::bitwise_or(mask_r, mask_g, mask);
    cv::bitwise_or(mask, mask_b, mask);

    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT, { 4, 4 }));
    auto mask_rect = cv::boundingRect(mask);
    if (mask_rect.empty()) {
        LogWarn << "Depot quantity mask is empty" << VAR(item.item_id);
        return std::nullopt;
    }
    mask_rect.width -= 1;
    mask_rect.height -= 1;

    mask_rect.height = std::max(mask_rect.height, 18);
    const auto mid_x = mask.cols / 2; // center of image
    if (mask_rect.br().x - mid_x < 30) {
        mask_rect.width = mid_x + 30 - mask_rect.x;
    }

    // Trim the bottom border, but keep the right edge so the last digit is not truncated.
    Rect ocr_roi { item.rect.x + mask_rect.x, item.rect.y + mask_rect.y, mask_rect.width, mask_rect.height - 2 };
    if (ocr_roi.width <= 0 || ocr_roi.height <= 0 || !image_rect.include(ocr_roi)) {
        LogWarn << "Invalid depot quantity OCR region" << VAR(item.item_id) << VAR(ocr_roi);
        return std::nullopt;
    }

    // Keep the original pixels for the small quantity text; only icon matching needs the resized image.
    const Rect raw_ocr_roi = resize_rect_to_raw_size(ocr_roi);
    const Rect raw_image_rect { 0, 0, m_image.cols, m_image.rows };
    if (raw_ocr_roi.width <= 0 || raw_ocr_roi.height <= 0 || !raw_image_rect.include(raw_ocr_roi)) {
        LogWarn << "Invalid depot quantity raw OCR region" << VAR(item.item_id) << VAR(raw_ocr_roi);
        return std::nullopt;
    }

    RegionOCRer analyzer(m_image);
    analyzer.set_task_info("NumberOcrReplace");
    analyzer.set_roi(raw_ocr_roi);
    analyzer.set_bin_threshold(task_ptr->special_params[0], task_ptr->special_params[1]);
    analyzer.set_use_char_model(false);
    analyzer.set_use_raw(false);

    if (!analyzer.analyze()) {
        return std::nullopt;
    }

    auto result = analyzer.get_result();
    constexpr double MinimumQuantityScore = 0.9;
    if (!(result.score >= MinimumQuantityScore)) {
        if (!std::isfinite(result.score) || result.text.empty() ||
            !std::ranges::all_of(result.text, [](const unsigned char c) { return c >= '0' && c <= '9'; })) {
            LogWarn << "Depot quantity OCR confidence is insufficient" << VAR(item.item_id) << VAR(result.score);
            return std::nullopt;
        }

        // The numeric model cannot read localized units. Only reread plain integers and require agreement.
        analyzer.set_use_char_model(true);
        analyzer.set_use_raw(true);
        const auto reread = analyzer.analyze();
        if (!reread || !(reread->score >= MinimumQuantityScore) || reread->text != result.text) {
            LogWarn << "Depot quantity numeric reread was not confirmed" << VAR(item.item_id) << VAR(result.text);
            return std::nullopt;
        }
        LogInfo << "Depot quantity numeric reread confirmed" << VAR(item.item_id) << VAR(reread->score);
        result = *reread;
    }

#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw_resized, make_rect<cv::Rect>(ocr_roi), cv::Scalar(0, 0, 255));
    cv::putText(
        m_image_draw_resized,
        result.text,
        cv::Point(ocr_roi.x, ocr_roi.y - 5),
        cv::FONT_HERSHEY_SIMPLEX,
        0.5,
        cv::Scalar(0, 255, 0),
        2);
#endif

    std::string digit_str = result.text;
    int multiple = 1;
    if (digit_str.ends_with("万")) {
        multiple = 10'000;
        digit_str.resize(digit_str.size() - std::string_view("万").size());
    }
    /*
    // 更新：这个模型改成了 ASCII 识别，所以识别不到万了，得到的结果是 1.35 这种形式，表示 1.3 万
    else if (size_t w2_pos = digit_str.find('.');
             digit_str.size() > 1 && w2_pos != std::string::npos && digit_str[digit_str.size() - 1] == '5') {
        multiple = 10000;
        digit_str.erase(digit_str.size() - 1, digit_str.size());
    }
    // 没小数点的时候万会识别成 F（怎么还会和上面不一样的
    else if (digit_str.ends_with('F')) {
        multiple = 10000;
        digit_str.erase(digit_str.size() - 1);
    }
    */
    else if (digit_str.ends_with('K')) {
        multiple = 1000;
        digit_str.pop_back();
    }
    else if (digit_str.ends_with("亿")) {
        multiple = 100'000'000;
        digit_str.resize(digit_str.size() - std::string_view("亿").size());
    }
    else if (digit_str.ends_with('M')) {
        multiple = 1'000'000;
        digit_str.pop_back();
    }
    else if (digit_str.ends_with("만")) {
        multiple = 10'000;
        digit_str.resize(digit_str.size() - std::string_view("만").size());
    }
    else if (digit_str.ends_with("억")) {
        multiple = 100'000'000;
        digit_str.resize(digit_str.size() - std::string_view("억").size());
    }

    constexpr char Dot = '.';
    if (digit_str.empty() ||
        !std::ranges::all_of(digit_str, [](const unsigned char c) -> bool { return std::isdigit(c) || c == Dot; })) {
        return std::nullopt;
    }
    if (auto dot_pos = digit_str.find(Dot); dot_pos != std::string::npos) {
        if (multiple == 1 || dot_pos == 0 || dot_pos == digit_str.size() - 1 ||
            digit_str.find(Dot, dot_pos + 1) != std::string::npos) {
            return std::nullopt;
        }
    }

    double number = 0;
    const auto [ptr, ec] = std::from_chars(digit_str.data(), digit_str.data() + digit_str.size(), number);
    if (ec != std::errc() || ptr != digit_str.data() + digit_str.size() ||
        number * multiple > std::numeric_limits<int>::max()) {
        return std::nullopt;
    }
    int quantity = static_cast<int>(number * multiple);
    LogInfo << "Depot item quantity" << VAR(quantity);
    return quantity;
}

asst::Rect asst::DepotImageAnalyzer::resize_rect_to_raw_size(const asst::Rect& rect)
{
    LogTraceFunction;

    m_resized_rect = Task.get("DepotMatchData")->roi;

    double kx = static_cast<double>(m_image.cols) / m_resized_rect.width;
    double ky = static_cast<double>(m_image.rows) / m_resized_rect.height;

    Rect raw_rect;
    raw_rect.x = static_cast<int>(kx * rect.x);
    raw_rect.y = static_cast<int>(ky * rect.y);
    raw_rect.width = static_cast<int>(kx * rect.width);
    raw_rect.height = static_cast<int>(ky * rect.height);

    return raw_rect;
}
