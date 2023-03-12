#include "StageDropsImageAnalyzer.h"

#include "Utils/Ranges.hpp"

#include "Utils/NoWarningCV.h"

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/Miscellaneous/StageDropsConfig.h"
#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/MatchImageAnalyzer.h"
#include "Vision/OcrWithFlagTemplImageAnalyzer.h"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

#include <numbers>

bool asst::StageDropsImageAnalyzer::analyze()
{
    LogTraceFunction;

    analyze_stage_code();
    analyze_difficulty();
    analyze_stars();
    bool ret = analyze_drops() && analyze_drops_for_CF();

#ifndef ASST_DEBUG
    if (!ret)
#endif // ! ASST_DEBUG
        save_img(utils::path("debug") / utils::path("drops"));

    return ret;
}

asst::StageKey asst::StageDropsImageAnalyzer::get_stage_key() const
{
    return { m_stage_code, m_difficulty };
}

int asst::StageDropsImageAnalyzer::get_stars() const noexcept
{
    return m_stars;
}

bool asst::StageDropsImageAnalyzer::analyze_stage_code()
{
    LogTraceFunction;

    OcrImageAnalyzer analyzer(m_image);
    analyzer.set_task_info("StageDrops-StageName");
    if (!analyzer.analyze()) {
        return false;
    }
    m_stage_code = analyzer.get_result().front().text;
    Log.info(__FUNCTION__, "stage_code", m_stage_code);

#ifdef ASST_DEBUG
    const Rect& text_rect = analyzer.get_result().front().rect;
    cv::rectangle(m_image_draw, make_rect<cv::Rect>(text_rect), cv::Scalar(0, 0, 255), 2);
    cv::putText(m_image_draw, m_stage_code, cv::Point(text_rect.x, text_rect.y - 10), cv::FONT_HERSHEY_SIMPLEX, 1,
                cv::Scalar(0, 0, 255), 2);
#endif

    return true;
}

bool asst::StageDropsImageAnalyzer::analyze_stars()
{
    LogTraceFunction;

    static const std::unordered_map<int, std::string> StarsTaskName = {
        { 2, "StageDrops-Stars-2" },
        { 3, "StageDrops-Stars-3" },
    };

    MatchImageAnalyzer analyzer(m_image);
    int matched_stars = 0;
    double max_score = 0.0;

#ifdef ASST_DEBUG
    Rect matched_rect(72, 292, 205, 58);
#endif

    for (const auto& [stars, task_name] : StarsTaskName) {
        auto task_ptr = Task.get(task_name);
        analyzer.set_task_info(task_name);

        if (!analyzer.analyze()) {
            continue;
        }

        if (auto score = analyzer.get_result().score; score > max_score) {
            max_score = score;
            matched_stars = stars;
#ifdef ASST_DEBUG
            matched_rect = analyzer.get_result().rect;
#endif
        }
    }
    m_stars = matched_stars;

    Log.info(__FUNCTION__, "stars", m_stars);

#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw, make_rect<cv::Rect>(matched_rect), cv::Scalar(0, 0, 255), 2);
    cv::putText(m_image_draw, std::to_string(m_stars) + " stars",
                cv::Point(matched_rect.x, matched_rect.y + matched_rect.height + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                cv::Scalar(0, 0, 255), 2);
#endif

    return true;
}

bool asst::StageDropsImageAnalyzer::analyze_difficulty()
{
    LogTraceFunction;

    static const std::unordered_map<std::string, StageDifficulty> DifficultyTaskName = {
        { "StageDrops-Difficulty-Normal", StageDifficulty::Normal },
        { "StageDrops-Difficulty-Normal2", StageDifficulty::Normal },
        { "StageDrops-Difficulty-Tough", StageDifficulty::Tough },
    };

    MatchImageAnalyzer analyzer(m_image);
    auto matched = StageDifficulty::Normal;
    double max_score = 0.0;

#ifdef ASST_DEBUG
    std::string matched_name = "unknown_difficulty";
    Rect matched_rect;
#endif

    for (const auto& [task_name, difficulty] : DifficultyTaskName) {
        auto task_ptr = Task.get(task_name);
        analyzer.set_task_info(task_name);

        if (!analyzer.analyze()) {
            continue;
        }

        if (auto score = analyzer.get_result().score; score > max_score) {
            max_score = score;
            matched = difficulty;
#ifdef ASST_DEBUG
            matched_name = task_name;
            matched_rect = analyzer.get_result().rect;
#endif
        }
    }
    m_difficulty = matched;
    Log.info(__FUNCTION__, "difficulty", static_cast<int>(m_difficulty));

#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw, make_rect<cv::Rect>(matched_rect), cv::Scalar(0, 0, 255), 2);
    matched_name = matched_name.substr(matched_name.find_last_of('-') + 1, matched_name.size());
    cv::putText(m_image_draw, matched_name, cv::Point(matched_rect.x, matched_rect.y + matched_rect.height + 20),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
#endif

    return true;
}

bool asst::StageDropsImageAnalyzer::analyze_drops()
{
    LogTraceFunction;

    if (!analyze_baseline()) {
        return false;
    }

    auto task_ptr = Task.get("StageDrops-Item");

    bool has_error = false;
    const auto& roi = task_ptr->roi;
    for (auto it = m_baseline.cbegin(); it != m_baseline.cend(); ++it) {
        const auto& [baseline, drop_type] = *it;
        bool is_first_drop_type = it == m_baseline.cbegin();
        int size = baseline.width / (roi.width + 10) + 1; // + 10 为了带点容差
        for (int i = 1; i <= size; ++i) {
            Rect item_roi;
            if (is_first_drop_type) {
                // 因为第一个黄色的 baseline 是渐变的，圈出来的一般左边会少一段，所以这里直接从右边开始往左推
                int x = baseline.x + baseline.width - i * roi.width;
                item_roi = Rect(x, baseline.y + roi.y, roi.width, roi.height);
            }
            else {
                // 其他的从左推
                int x = baseline.x + (i - 1) * roi.width;
                item_roi = Rect(x, baseline.y + roi.y, roi.width, roi.height);
            }

            std::string item = match_item(item_roi, drop_type, size - i, size);
            bool use_word_model = item == LMD_ID;
            int quantity = match_quantity(item_roi, item, use_word_model);
            if (use_word_model && quantity == 0) {
                quantity = match_quantity(item_roi, item, false);
            }
            Log.info("Item id:", item, ", quantity:", quantity);
#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(item_roi), cv::Scalar(0, 0, 255), 2);
            cv::putText(m_image_draw, item, cv::Point(item_roi.x, item_roi.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                        cv::Scalar(0, 0, 255), 2);
            cv::putText(m_image_draw, std::to_string(quantity), cv::Point(item_roi.x, item_roi.y + 10),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
#endif
            if (quantity <= 0) {
                has_error = true;
                Log.error(__FUNCTION__, "quantity error", quantity);
            }
            if (item.empty()) {
                Log.warn(__FUNCTION__, "item id is empty");
            }
            StageDropInfo info;
            info.drop_type = drop_type;
            info.item_id = std::move(item);
            info.quantity = quantity;

            const std::string& name = ItemData.get_item_name(info.item_id);
            info.item_name = name.empty() ? info.item_id : name;

            static const std::unordered_map<StageDropType, std::string> DropTypeName = {
                { StageDropType::Normal, "NORMAL_DROP" },     { StageDropType::Extra, "EXTRA_DROP" },
                { StageDropType::Furniture, "FURNITURE" },    { StageDropType::Special, "SPECIAL_DROP" },
                { StageDropType::ExpAndLMB, "EXP_LMB_DROP" }, { StageDropType::Sanity, "SANITY_DROP" },
                { StageDropType::Reward, "REWARD_DROP" },     { StageDropType::Unknown, "UNKNOWN_DROP" }
            };
            info.drop_type_name = DropTypeName.at(drop_type);

            m_drops.emplace_back(std::move(info));
        }
    }
    return !has_error;
}

bool asst::StageDropsImageAnalyzer::analyze_drops_for_CF()
{
    if (!m_stage_code.starts_with("CF-")) {
        return true;
    }
    LogTraceFunction;

    static const std::array<std::string, 5> CFDrops = { "act24side_melding_1", "act24side_melding_2",
                                                        "act24side_melding_3", "act24side_melding_4",
                                                        "act24side_melding_5" }; // "act24side_melding_6"

    bool has_error = false;

    OcrImageAnalyzer food_analyzer(m_image);
    food_analyzer.set_task_info("StageDrops-StageCF-FoodBonusFlag");
    if (food_analyzer.analyze()) {
        // 这个企鹅物流不收，而且也不好识别，直接报错拉倒
        Log.info(__FUNCTION__, "Food Bonus, stop to upload");
        has_error = true;
    }

    OcrWithFlagTemplImageAnalyzer analyzer(m_image);
    for (const auto& item_name : CFDrops) {
        analyzer.set_task_info(item_name, "StageDrops-StageCF-ItemQuantity");
        if (!analyzer.analyze()) {
            continue;
        }
        const auto& result = analyzer.get_result().front();
        int quantity = quantity_string_to_int(result.text);

        Log.info("Item id:", item_name, ", quantity:", quantity);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(result.rect), cv::Scalar(0, 0, 255), 2);
        cv::putText(m_image_draw, std::string("CF: ") + item_name.back(), cv::Point(result.rect.x, result.rect.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
        cv::putText(m_image_draw, std::to_string(quantity), cv::Point(result.rect.x, result.rect.y + 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
#endif
        if (quantity <= 0) {
            has_error = true;
            Log.error(__FUNCTION__, "quantity error", quantity);
        }
        StageDropInfo info;
        info.drop_type = StageDropType::Normal;
        info.drop_type_name = "NORMAL_DROP";
        info.quantity = quantity;
        info.item_id = item_name;
        const std::string& name = ItemData.get_item_name(info.item_id);
        info.item_name = name.empty() ? info.item_id : name;

        m_drops.emplace_back(std::move(info));
    }
    return !has_error;
}

bool asst::StageDropsImageAnalyzer::analyze_baseline()
{
    LogTraceFunction;

    auto task_ptr = Task.get<MatchTaskInfo>("StageDrops-BaseLine");

    cv::Mat preprocessed_roi;

    {
        double angle = 60. / 180. * std::numbers::pi;
        cv::Mat temp; // convert to float point for convenience
        m_image.convertTo(temp, CV_32F, 1. / 255);
        cv::Mat pdy;
        cv::Mat pdx;
        cv::Sobel(temp, pdy, CV_32F, 0, 1);
        cv::Sobel(temp, pdx, CV_32F, 1, 0);
        // prefer gradient in vertical direction, make value of those pixels positive
        cv::sqrt(pdy.mul(pdy) - std::pow(std::tan(angle), 2.) * pdx.mul(pdx), temp);

        temp.convertTo(preprocessed_roi, CV_8U, 255);

        // filling small gaps
        cv::dilate(preprocessed_roi, preprocessed_roi, cv::getStructuringElement(cv::MORPH_RECT, { 3, 1 }));
        // line must be thick enough
        cv::erode(preprocessed_roi, preprocessed_roi, cv::getStructuringElement(cv::MORPH_RECT, { 3, 2 }));

        // cropping after derivatives, dilation, and erosion
        cv::cvtColor(preprocessed_roi(make_rect<cv::Rect>(task_ptr->roi)), preprocessed_roi, cv::COLOR_BGR2GRAY);
    }

    cv::Mat preprocessed_bin;
    cv::inRange(preprocessed_roi, task_ptr->mask_range.first, task_ptr->mask_range.second, preprocessed_bin);

    cv::Rect bounding_rect = cv::boundingRect(preprocessed_bin);
    cv::Mat bounding = preprocessed_bin(bounding_rect);

    int x_offset = task_ptr->roi.x + bounding_rect.x;
    int y_offset = task_ptr->roi.y + bounding_rect.y;

    const int min_width = task_ptr->special_params.front();
    const int max_spacing = static_cast<int>(task_ptr->templ_threshold);

    int i_start = 0, i_end = bounding.cols - 1;
    bool in = true; // 是否正处在白线中
    int spacing = 0;

    // split
    for (int i = 0; i < bounding.cols; ++i) {
        uchar value = 0;
        for (int j = 0; j < bounding.rows; ++j) {
            value = std::max(value, bounding.at<uchar>(j, i));
        }

        bool is_white = value;

        if (in && !is_white) {
            in = false;
            i_end = i;
            int width = i_end - i_start;
            if (width < min_width) {
                spacing += i_end - i_start;
            }
            else {
                spacing = 0;
                Rect baseline { x_offset + i_start, y_offset, width, bounding_rect.height };
                m_baseline.emplace_back(baseline, match_droptype(baseline));
            }
        }
        else if (!in && is_white) {
            i_start = i;
            in = true;
        }
        else if (!in) {
            if (++spacing > max_spacing && i_start != 0) {
                break;
            }
        }
    }

    if (in) { // 最右边的白线贴着 bounding 边的情况
        int width = bounding.cols - 1 - i_start;
        if (width >= min_width) {
            Rect baseline { x_offset + i_start, y_offset, width, bounding_rect.height };
            m_baseline.emplace_back(baseline, match_droptype(baseline));
        }
    }
    if (!m_baseline.empty()) {
        if (m_baseline.back().second == StageDropType::Unknown) {
            Log.warn(__FUNCTION__, "The last baseline is unknown type, remove it");
            m_baseline.pop_back();
        }
    }

    Log.trace(__FUNCTION__, "baseline size", m_baseline.size());
    for (const auto& key : m_baseline | views::keys) {
        Log.trace(__FUNCTION__, "baseline", key.to_string());
    }

    return !m_baseline.empty();
}

asst::StageDropType asst::StageDropsImageAnalyzer::match_droptype(const Rect& roi)
{
    LogTraceFunction;

    Log.trace(__FUNCTION__, "baseline: ", roi.to_string());

    static const std::unordered_map<StageDropType, std::string> DropTypeTaskName = {
        { StageDropType::ExpAndLMB, "StageDrops-DropType-ExpAndLMB" },
        { StageDropType::Normal, "StageDrops-DropType-Normal" },
        { StageDropType::Extra, "StageDrops-DropType-Extra" },
        { StageDropType::Furniture, "StageDrops-DropType-Furniture" },
        { StageDropType::Special, "StageDrops-DropType-Special" },
        { StageDropType::Sanity, "StageDrops-DropType-Sanity" },
        { StageDropType::Reward, "StageDrops-DropType-Reward" },
    };

    MatchImageAnalyzer analyzer(m_image);
    auto matched = StageDropType::Unknown;
    double max_score = 0.0;

#ifdef ASST_DEBUG
    std::string matched_name;
    Rect matched_roi;
#endif

    for (const auto& [type, task_name] : DropTypeTaskName) {
        auto task_ptr = Task.get(task_name);
        analyzer.set_task_info(task_name);

        Rect droptype_roi = roi;
        droptype_roi.y += task_ptr->roi.y;
        droptype_roi.height = task_ptr->roi.height;
        analyzer.set_roi(droptype_roi);
        if (!analyzer.analyze()) {
            continue;
        }
        if (auto score = analyzer.get_result().score; score > max_score) {
            max_score = score;
            matched = type;
#ifdef ASST_DEBUG
            matched_name = task_name;
            matched_roi = droptype_roi;
#endif
        }
    }

#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw, make_rect<cv::Rect>(matched_roi), cv::Scalar(0, 0, 255), 2);
    matched_name = matched_name.substr(matched_name.find_last_of('-') + 1, matched_name.size());
    cv::putText(m_image_draw, matched_name, cv::Point(matched_roi.x, matched_roi.y + matched_roi.height + 20),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
#endif

    return matched;
}

std::string asst::StageDropsImageAnalyzer::match_item(const Rect& roi, StageDropType type, int index, int size)
{
    LogTraceFunction;

    switch (type) {
    case StageDropType::ExpAndLMB:
        if (size == 1) {
            return LMD_ID; // 龙门币
        }
        else if (size == 2) {
            if (index == 0) {
                return "5001"; // 声望（经验）
            }
            else {
                return LMD_ID; // 龙门币
            }
        }
        else {
            Log.error("StageDropType::ExpAndLMB, size", size);
            return {};
        }
        break;
    case StageDropType::Furniture:
        return "furni"; // 家具
    case StageDropType::Sanity:
        return "AP_GAMEPLAY"; // 理智返还
    case StageDropType::Reward:
        return "4003"; // 合成玉
    }

    auto match_item_with_templs = [&](const std::vector<std::string>& templs_list) -> std::string {
        MatchImageAnalyzer analyzer(m_image);
        analyzer.set_task_info("StageDrops-Item");
        analyzer.set_mask_with_close(true);
        analyzer.set_roi(roi);

        double max_score = 0.0;
        std::string matched;
        for (const std::string& templ : templs_list) {
            analyzer.set_templ_name(templ);
            if (!analyzer.analyze()) {
                continue;
            }
            if (auto score = analyzer.get_result().score; score > max_score) {
                max_score = score;
                matched = templ;
            }
        }
        return matched;
    };

    std::string result;
    if (!m_stage_code.empty()) {
        auto& drops = StageDrops.get_stage_info(m_stage_code, m_difficulty).drops;
        if (auto find_iter = drops.find(type); find_iter != drops.end()) {
            result = match_item_with_templs(find_iter->second);
        }
    }

    // 没识别到的话就把全部材料都拿来跑一遍
    if (result.empty()) {
        auto items = ItemData.get_all_item_id();
        result = match_item_with_templs(std::vector<std::string>(items.cbegin(), items.cend()));
        // 将这次识别到的加入该关卡的待识别列表
        if (!result.empty() && !m_stage_code.empty()) {
            StageDrops.append_drops(StageKey { m_stage_code, m_difficulty }, type, result);
        }
    }

    return result;
}

std::optional<asst::TextRect> asst::StageDropsImageAnalyzer::match_quantity_string(const asst::Rect& roi,
                                                                                   bool use_word_model)
{
    auto task_ptr = Task.get<MatchTaskInfo>("StageDrops-Quantity");

    Rect quantity_roi = roi.move(task_ptr->roi);
    cv::Mat quantity_img = m_image(make_rect<cv::Rect>(quantity_roi));

    cv::Mat gray;
    cv::cvtColor(quantity_img, gray, cv::COLOR_BGR2GRAY);
    cv::Mat bin;
    cv::inRange(gray, task_ptr->mask_range.first, task_ptr->mask_range.second, bin);

    // split
    const int max_spacing = static_cast<int>(task_ptr->templ_threshold);
    std::vector<cv::Range> contours;
    int i_right = bin.cols - 1, i_left = 0;
    bool in = false;
    int spacing = 0;

    for (int i = bin.cols - 1; i >= 0; --i) {
        bool has_white = false;
        for (int j = 0; j < bin.rows; ++j) {
            if (bin.at<uchar>(j, i)) {
                has_white = true;
                break;
            }
        }
        if (in && !has_white) {
            i_left = i;
            in = false;
            spacing = 0;
            contours.emplace_back(i_left, i_right + 1); // range 是前闭后开的
        }
        else if (!in && has_white) {
            i_right = i;
            in = true;
        }
        else if (!in) {
            if (++spacing > max_spacing && i_left != 0) {
                // filter out noise
                break;
            }
        }
    }

    if (contours.empty()) {
        return std::nullopt;
    }

    // 前面的 split 算法经过了大量的测试集验证，分割效果一切正常
    // 后来发现不需要 split 了（为了优化识别速度），但是这个算法不太敢动，怕出问题
    // 所以这里保留前面的 split 算法，直接取两侧端点。（反正前面跑几个循环也费不了多长时间）
    int far_left = contours.back().start;
    int far_right = contours.front().end;

    OcrWithPreprocessImageAnalyzer analyzer(m_image);
    analyzer.set_task_info("NumberOcrReplace");
    analyzer.set_roi(Rect(quantity_roi.x + far_left, quantity_roi.y, far_right - far_left, quantity_roi.height));
    analyzer.set_threshold(task_ptr->mask_range.first, task_ptr->mask_range.second);
    analyzer.set_use_char_model(!use_word_model);

    if (!analyzer.analyze()) {
        return std::nullopt;
    }

    return analyzer.get_result().front();
}

std::optional<asst::TextRect> asst::StageDropsImageAnalyzer::match_quantity_string(const asst::Rect& roi,
                                                                                   const std::string& item,
                                                                                   bool use_word_model)
{
    auto task_ptr = Task.get<MatchTaskInfo>("StageDrops-Quantity");
    auto templ = TemplResource::get_instance().get_templ(item).clone();
    if (templ.empty()) {
        Log.error("templ is empty: ", item);
        return std::nullopt;
    }

    MatchImageAnalyzer analyzer(m_image);
    analyzer.set_templ(templ);
    analyzer.set_mask_range(1, 255);
    analyzer.set_mask_with_close(true);
    analyzer.set_roi(roi);
    if (!analyzer.analyze()) {
        return std::nullopt;
    }
    Rect new_roi = analyzer.get_result().rect;
    cv::Mat item_img = m_image(make_rect<cv::Rect>(new_roi));

    // ref: DepotImageAnalyzer::match_quantity
    cv::Mat quotient;
    cv::divide(item_img + cv::Scalar { 1, 1, 1 }, templ + cv::Scalar { 1, 1, 1 }, quotient, 255);

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

    cv::Mat templ_mask;
    cv::inRange(templ, cv::Scalar { 0, 0, 0 }, cv::Scalar { 0, 0, 0 }, templ_mask);
    cv::bitwise_not(templ_mask, templ_mask);
    cv::bitwise_and(mask, templ_mask, mask);
    mask(cv::Rect { 0, 0, mask.cols / 4, mask.rows }) = cv::Scalar { 0, 0, 0 };
    mask(cv::Rect { 0, 0, mask.cols, mask.rows * 2 / 3 }) = cv::Scalar { 0, 0, 0 };
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT, { 4, 4 }));

    auto mask_rect = cv::boundingRect(mask);
    mask_rect.width -= 1;
    mask_rect.height -= 1;
    if (mask_rect.height < 20) mask_rect.height = 20;

    cv::Mat ocr_img = m_image.clone();
    cv::subtract(ocr_img(make_rect<cv::Rect>(new_roi)), templ * 0.41, ocr_img(make_rect<cv::Rect>(new_roi)));

    OcrWithPreprocessImageAnalyzer ocr(ocr_img);
    ocr.set_task_info("NumberOcrReplace");
    Rect ocr_roi { new_roi.x + mask_rect.x, new_roi.y + mask_rect.y, mask_rect.width, mask_rect.height };
    ocr.set_roi(ocr_roi);
    ocr.set_use_char_model(!use_word_model);
    ocr.set_threshold(task_ptr->mask_range.first, task_ptr->mask_range.second);

    if (!ocr.analyze()) {
        return std::nullopt;
    }

    return ocr.get_result().front();
}

int asst::StageDropsImageAnalyzer::quantity_string_to_int(const std::string& str)
{
    std::string digit_str = str;
    int multiple = 1;
    if (size_t w_pos = digit_str.find("万"); w_pos != std::string::npos) {
        multiple = 10000;
        digit_str.erase(w_pos, digit_str.size());
    }
    else if (size_t k_pos = digit_str.find('k'); k_pos != std::string::npos) {
        multiple = 1000;
        digit_str.erase(k_pos, digit_str.size());
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

int asst::StageDropsImageAnalyzer::match_quantity(const asst::Rect& roi, const std::string& item, bool use_word_model)
{
    TextRect result;
    // is furniture?
    if (item.empty() || item == "furni") {
        auto opt = match_quantity_string(roi, use_word_model);
        if (!opt) return 0;
        result = opt.value();
    }
    else {
        auto opt = match_quantity_string(roi, item, use_word_model);
        if (!opt) return 0;
        result = opt.value();
    }

#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw, make_rect<cv::Rect>(result.rect), cv::Scalar(0, 0, 255));
    if (use_word_model) {
        cv::putText(m_image_draw, result.text, cv::Point(result.rect.x, result.rect.y - 20), cv::FONT_HERSHEY_SIMPLEX,
                    0.5, cv::Scalar(0, 0, 255), 2);
    }
    else {
        cv::putText(m_image_draw, result.text, cv::Point(result.rect.x, result.rect.y - 5), cv::FONT_HERSHEY_SIMPLEX,
                    0.5, cv::Scalar(0, 255, 0), 2);
    }
#endif

    return quantity_string_to_int(result.text);
}
