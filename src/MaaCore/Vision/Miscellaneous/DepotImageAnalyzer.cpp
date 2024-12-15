#include "DepotImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

#include <numbers>

bool asst::DepotImageAnalyzer::analyze()
{
    LogTraceFunction;

    m_all_items_roi.clear();
    m_result.clear();

    // 因为模板素材的尺寸与实际截图中素材尺寸不符，所以这里先对原图进行一下缩放
    resize();
    bool ret = analyze_base_rect();
    if (!ret) {
        return false;
    }

    ret = analyze_all_items();

#ifdef ASST_DEBUG
    m_image_draw = m_image_draw_resized;
#endif
    save_img(utils::path("debug") / utils::path("depot"));
    return ret;
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
        if (check_roi_empty(roi)) { // roi 是竖着有序的
            break;
        }
        ItemInfo info;
        size_t cur_pos = match_item(roi, info, m_match_begin_pos);
        if (cur_pos == NPos) {
            break;
        }
        std::string item_id = info.item_id;

        m_match_begin_pos = cur_pos + 1;
        info.quantity = match_quantity(info);
        info.item_name = ItemData.get_item_name(item_id);
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
        if (item_id.empty() || info.quantity == 0) {
            Log.error(__FUNCTION__, item_id, info.item_name, " quantity is zero");
            continue;
        }
        info.rect = resize_rect_to_raw_size(info.rect);
        m_result.emplace(std::move(item_id), std::move(info));
    }
#ifdef ASST_DEBUG
    cv::Mat hsv;
    cv::cvtColor(m_image_resized, hsv, cv::COLOR_BGR2HSV);
#endif

    return !m_result.empty();
}

bool asst::DepotImageAnalyzer::check_roi_empty(const Rect& roi)
{
    // TODO
    std::ignore = roi;
    return false;
}

size_t asst::DepotImageAnalyzer::match_item(
    const Rect& roi,
    /* out */ ItemInfo& item_info,
    size_t begin_index,
    bool with_enlarge)
{
    LogTraceFunction;

    const auto& all_items = ItemData.get_ordered_material_item_id();

    Matcher analyzer(m_image_resized);
    analyzer.set_task_info("DepotMatchData");
    // spacing 有时候算的差一个像素，干脆把 roi 扩大一点好了
    Rect enlarged_roi = roi;
    if (with_enlarge) {
        enlarged_roi = Rect(roi.x - 20, roi.y - 5, roi.width + 40, roi.height + 10);
    }
    analyzer.set_roi(enlarged_roi);

    MatchRect matched;
    std::string matched_item_id;
    size_t matched_index = NPos;
    for (size_t index = begin_index, extra_count = 0; index < all_items.size(); ++index) {
        const std::string& item_id = all_items.at(index);
        // analyzer.set_templ(item_id);
        // TODO: too slow? find a way to set mask directly
        cv::Mat templ = TemplResource::get_instance().get_templ(item_id).clone();
        templ(cv::Rect { templ.cols - 80, templ.rows - 50, 80, 50 }) = cv::Scalar { 0, 0, 0 };
        analyzer.set_templ(templ);
        if (!analyzer.analyze()) {
            continue;
        }
        if (double score = analyzer.get_result().score; score >= matched.score) {
            matched = analyzer.get_result();
            matched_item_id = item_id;
            matched_index = index;
        }
        // 匹配到了任一结果后，再往后匹配几个。
        // 因为有些相邻的材料长得很像（同一种类的）
        constexpr size_t MaxExtraMatch = 8;
        if (matched_index != NPos && ++extra_count >= MaxExtraMatch) {
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

int asst::DepotImageAnalyzer::match_quantity(const ItemInfo& item)
{
    auto task_ptr = Task.get<MatchTaskInfo>("DepotQuantity");
    auto item_templ = TemplResource::get_instance().get_templ(item.item_id);
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
    mask_rect.width -= 1;
    mask_rect.height -= 1;

    if (mask_rect.height < 18) {
        mask_rect.height = 18;
    }
    const auto mid_x = mask.cols / 2; // center of image
    if (mask_rect.br().x - mid_x < 30) {
        mask_rect.width = mid_x + 30 - mask_rect.x;
    }

    // minus 2 to trim white pixels
    Rect ocr_roi { item.rect.x + mask_rect.x, item.rect.y + mask_rect.y, mask_rect.width - 2, mask_rect.height - 2 };

    cv::Mat ocr_img = m_image_resized.clone();
    cv::subtract(
        m_image_resized(make_rect<cv::Rect>(item.rect)),
        item_templ * 0.41,
        ocr_img(make_rect<cv::Rect>(item.rect)));

    RegionOCRer analyzer(m_image_resized);
    analyzer.set_task_info("NumberOcrReplace");
    analyzer.set_roi(ocr_roi);
    analyzer.set_bin_threshold(task_ptr->special_params[0], task_ptr->special_params[1]);

    if (!analyzer.analyze()) {
        return 0;
    }

    const auto& result = analyzer.get_result();

#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw_resized, make_rect<cv::Rect>(result.rect), cv::Scalar(0, 0, 255));
    cv::putText(
        m_image_draw_resized,
        result.text,
        cv::Point(result.rect.x, result.rect.y - 5),
        cv::FONT_HERSHEY_SIMPLEX,
        0.5,
        cv::Scalar(0, 255, 0),
        2);
#endif

    std::string digit_str = result.text;
    int multiple = 1;
    if (size_t w_pos = digit_str.find("万"); w_pos != std::string::npos) {
        multiple = 10000;
        digit_str.erase(w_pos, digit_str.size());
    }
    // 更新：这个模型改成了 ASCII 识别，所以识别不到万了，得到的结果是 1.35 这种形式，表示 1.3 万
    else if (size_t w2_pos = digit_str.find('.');
             digit_str.size() > 1 && w2_pos != std::string::npos && digit_str[digit_str.size() - 1] == '5') {
        multiple = 10000;
        digit_str.erase(digit_str.size() - 1, digit_str.size());
    }
    else if (size_t k_pos = digit_str.find('K'); k_pos != std::string::npos) {
        multiple = 1000;
        digit_str.erase(k_pos, digit_str.size());
    }
    else if (size_t e_pos = digit_str.find("亿"); e_pos != std::string::npos) {
        multiple = 100'000'000;
        digit_str.erase(e_pos, digit_str.size());
    }
    else if (size_t m_pos = digit_str.find('M'); m_pos != std::string::npos) {
        multiple = 1'000'000;
        digit_str.erase(m_pos, digit_str.size());
    }
    else if (size_t n_pos = digit_str.find("만"); n_pos != std::string::npos) {
        multiple = 10000;
        digit_str.erase(n_pos, digit_str.size());
    }
    else if (size_t o_pos = digit_str.find("억"); o_pos != std::string::npos) {
        multiple = 100'000'000;
        digit_str.erase(o_pos, digit_str.size());
    }

    constexpr char Dot = '.';
    if (digit_str.empty() ||
        !ranges::all_of(digit_str, [](const char& c) -> bool { return std::isdigit(c) || c == Dot; })) {
        return 0;
    }
    if (auto dot_pos = digit_str.find(Dot); dot_pos != std::string::npos) {
        if (dot_pos == 0 || dot_pos == digit_str.size() - 1 || digit_str.find(Dot, dot_pos + 1) != std::string::npos) {
            return 0;
        }
    }

    int quantity = static_cast<int>(std::stod(digit_str) * multiple);
    Log.info("Quantity:", quantity);
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
