#include "StageDropsImageAnalyzer.h"

#include "TaskData.h"
#include "OcrImageAnalyzer.h"
#include "MatchImageAnalyzer.h"
#include "Resource.h"
#include "AsstUtils.hpp"
#include "Logger.hpp"

#ifndef  ASST_DEBUG
#define ASST_DEBUG
#endif // ! ASST_DEBUG

bool asst::StageDropsImageAnalyzer::analyze()
{
    LogTraceFunction;

#ifdef ASST_DEBUG
    std::string stem = utils::get_format_time();
    stem = utils::string_replace_all_batch(stem, { {":", "-"}, {" ", "_"} });
    std::filesystem::create_directory("debug");
    cv::imwrite("debug/" + stem + "_raw.png", m_image);
#endif

    analyze_stage_name();
    analyze_difficulty();
    analyze_drops();

#ifdef ASST_DEBUG
    cv::imwrite("debug/" + stem + "_draw.png", m_image_draw);
#endif

    return false;
}

bool asst::StageDropsImageAnalyzer::analyze_stage_name()
{
    LogTraceFunction;

    OcrImageAnalyzer analyzer(m_image);
    analyzer.set_task_info("StageDrops-StageName");
    const auto& stages = Resrc.drops().get_all_stage_code();
    std::vector<std::string> stages_req(stages.cbegin(), stages.cend());
    // 名字长的放前面
    std::sort(stages_req.begin(), stages_req.end(),
        [](const std::string& lhs, const std::string& rhs) -> bool {
            return lhs.size() > rhs.size();
        });
    analyzer.set_required(std::move(stages_req));

    if (!analyzer.analyze()) {
        return false;
    }
    m_stage_name = analyzer.get_result().front().text;

#ifdef ASST_DEBUG
    const Rect& text_rect = analyzer.get_result().front().rect;
    cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(text_rect), cv::Scalar(0, 0, 255), 2);
    cv::putText(m_image_draw, m_stage_name, cv::Point(text_rect.x, text_rect.y - 10), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
#endif

    return true;
}

bool asst::StageDropsImageAnalyzer::analyze_difficulty()
{
    LogTraceFunction;

    static const std::unordered_map<StageDifficulty, std::string> DifficultyTaskName = {
        {StageDifficulty::Normal, "StageDrops-Difficulty-Normal"},
        {StageDifficulty::Tough, "StageDrops-Difficulty-Tough"},
    };

    MatchImageAnalyzer analyzer(m_image);
    StageDifficulty matched = StageDifficulty::Normal;
    double max_score = 0.0;

#ifdef ASST_DEBUG
    std::string matched_name = "unknown_difficulty";
    Rect matched_rect;
#endif

    for (const auto& [difficulty, task_name] : DifficultyTaskName) {
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

#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(matched_rect), cv::Scalar(0, 0, 255), 2);
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

    const auto& roi = task_ptr->roi;
    for (const auto& [baseline, droptype] : m_baseline) {
        int size = baseline.width / (roi.width + 10) + 1;   // + 10 为了带点容差
        for (int i = 1; i <= size; ++i) {
            // 因为第一个黄色的 baseline 是渐变的，圈出来的一般左边会少一段，所以这里直接从右边开始往左推
            int x = baseline.x + baseline.width - i * roi.width;
            Rect item_roi = Rect(x, baseline.y + roi.y, roi.width, roi.height);

            std::string item = match_item(item_roi, droptype, size - i, size);
            int quantity = match_quantity(item_roi);
            Log.info("Item id:", item, ", quantity:", quantity);
#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(item_roi), cv::Scalar(0, 0, 255), 2);
            cv::putText(m_image_draw, item, cv::Point(item_roi.x, item_roi.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
            cv::putText(m_image_draw, std::to_string(quantity), cv::Point(item_roi.x, item_roi.y + 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
#endif
        }
    }
    return true;
}

bool asst::StageDropsImageAnalyzer::analyze_baseline()
{
    LogTraceFunction;

    auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        Task.get("StageDrops-BaseLine"));

    cv::Mat roi = m_image(utils::make_rect<cv::Rect>(task_ptr->roi));
    cv::Mat gray;
    cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    cv::Mat bin;
    cv::inRange(gray, task_ptr->mask_range.first, task_ptr->mask_range.second, bin);

    cv::Rect bounding_rect = cv::boundingRect(bin);
    cv::Mat bounding = bin(bounding_rect);

    int x_offset = task_ptr->roi.x + bounding_rect.x;
    int y_offset = task_ptr->roi.y + bounding_rect.y;

    // split
    int istart = 0, iend = bounding.cols - 1;
    bool in = true;
    int not_in_count = 0;
    for (int i = 0; i < bounding.cols; ++i) {
        bool has_white = false;
        for (int j = 0; j < bounding.rows; ++j) {
            if (bounding.at<uchar>(j, i)) {
                has_white = true;
                break;
            }
        }
        if (in && !has_white) {
            in = false;
            iend = i;
            if (iend - istart < task_ptr->special_threshold) {
                not_in_count += iend - istart;
            }
            else {
                not_in_count = 0;
                Rect baseline{ x_offset + istart, y_offset, iend - istart, bounding_rect.height };
                m_baseline.emplace_back(baseline, match_droptype(baseline));
            }
        }
        else if (!in && has_white) {
            istart = i;
            in = true;
        }
        else if (!in) {
            if (++not_in_count > task_ptr->templ_threshold &&
                istart != 0) {
                // filter out noise
                break;
            }
        }
    }
    if (in) {
        Rect baseline{ x_offset + istart, y_offset, bounding.cols - 1 - istart, bounding_rect.height };
        m_baseline.emplace_back(baseline, match_droptype(baseline));
    }

    //m_baseline.erase(std::remove_if(m_baseline.begin(), m_baseline.end(), [&](const auto& baseline) {
    //    return baseline.first.width < task_ptr->special_threshold;
    //    }),
    //    m_baseline.end());

    return !m_baseline.empty();
}

asst::StageDropType asst::StageDropsImageAnalyzer::match_droptype(const Rect& roi)
{
    LogTraceFunction;

    Log.trace(__FUNCTION__, "baseline: ", roi.to_string());

    static const std::unordered_map<StageDropType, std::string> DropTypeTaskName = {
        {StageDropType::ExpAndLMB, "StageDrops-DropType-ExpAndLMB"},
        {StageDropType::Normal, "StageDrops-DropType-Normal"},
        {StageDropType::Extra, "StageDrops-DropType-Extra"},
        {StageDropType::Funriture, "StageDrops-DropType-Funriture"},
        {StageDropType::Special, "StageDrops-DropType-Special"},
        {StageDropType::Sanity, "StageDrops-DropType-Sanity"},
        {StageDropType::Reward, "StageDrops-DropType-Reward"},
    };

    MatchImageAnalyzer analyzer(m_image);
    StageDropType matched = StageDropType::Unknown;
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
    cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(matched_roi), cv::Scalar(0, 0, 255), 2);
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
            return "4001"; // 龙门币
        }
        else if (size == 2) {
            if (index == 0) {
                return "5001";  // 声望（经验）
            }
            else {
                return "4001";  // 龙门币
            }
        }
        else {
            Log.error("StageDropType::ExpAndLMB, size", size);
            return std::string();
        }
        break;
    case StageDropType::Funriture:
        return "funriture";     // 家具
    case StageDropType::Sanity:
        return "AP_GAMEPLAY";   // 理智返还
    case StageDropType::Reward:
        return "4003";          // 合成玉
    }

    auto match_item_with_templs = [&](std::vector<std::string> templs_list) -> std::string {
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
    if (!m_stage_name.empty()) {
        auto& drops = Resrc.drops().get_stage_info(m_stage_name, m_difficulty).drops;
        if (auto find_iter = drops.find(type); find_iter != drops.end()) {
            result = match_item_with_templs(find_iter->second);
        }
    }

    if (result.empty()) {
        auto items = Resrc.item().get_all_item_id();
        result = match_item_with_templs(std::vector<std::string>(items.cbegin(), items.cend()));
    }

    return result;
}

int asst::StageDropsImageAnalyzer::match_quantity(const Rect& roi)
{
    auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        Task.get("StageDrops-Quantity"));

    Rect quantity_roi = roi.move(task_ptr->roi);
    cv::Mat quantity_img = m_image(utils::make_rect<cv::Rect>(quantity_roi));

    cv::Mat gray;
    cv::cvtColor(quantity_img, gray, cv::COLOR_BGR2GRAY);
    cv::Mat bin;
    cv::inRange(gray, task_ptr->mask_range.first, task_ptr->mask_range.second, bin);

    std::vector<cv::Range> contours;
    // split and filter out noise
    int iright = bin.cols - 1, ileft = 0;
    bool in = false;
    int not_in_count = 0;
    for (int i = bin.cols - 1; i >= 0; --i) {
        bool has_white = false;
        for (int j = 0; j < bin.rows; ++j) {
            if (bin.at<uchar>(j, i)) {
                has_white = true;
                break;
            }
        }
        if (in && !has_white) {
            ileft = i;
            in = false;
            not_in_count = 0;
            contours.emplace_back(ileft, iright + 1);   // range 是前闭后开的
        }
        else if (!in && has_white) {
            iright = i;
            in = true;
        }
        else if (!in) {
            if (++not_in_count > task_ptr->templ_threshold &&
                ileft != 0) {
                // filter out noise
                break;
            }
        }
    }

    OcrImageAnalyzer analyzer(m_image);
    analyzer.set_use_cache(true);

    int quantity = 0;
    for (auto riter = contours.crbegin(); riter != contours.crend(); ++riter) {
        auto& range = *riter;
        cv::Mat range_img = bin(cv::Range::all(), range);
        cv::Rect rect = cv::boundingRect(range_img);
        cv::Mat bounding = range_img(rect);

        Rect absolute_rect;
        absolute_rect.x = quantity_roi.x + rect.x + range.start - 1;
        absolute_rect.y = quantity_roi.y + rect.y - 1;
        absolute_rect.width = rect.width + 2;
        absolute_rect.height = rect.height + 2;

#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(absolute_rect), cv::Scalar(0, 255, 0), 1);
#endif

        analyzer.set_region_of_appeared(absolute_rect);
        if (!analyzer.analyze()) {
            return 0;
        }
        auto digit_str = analyzer.get_result().front().text;
#ifdef ASST_DEBUG
        cv::putText(m_image_draw, digit_str, cv::Point(absolute_rect.x, absolute_rect.y - 5),
            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
#endif
        int dot = 0;
        if (digit_str == "万") {
            quantity *= 10000;
        }
        else if (digit_str == "亿") {
            quantity *= 100000000;
        }
        else if (digit_str.size() == 1) {
            if (char c = digit_str[0]; c >= '0' && c <= '9') {
                quantity *= 10;
                quantity += c - '0';
                if (dot) {
                    ++dot;
                }
            }
            else if (c == '.') {
                dot = 1;
            }
        }
        else {
            Log.error("quantity is not a single digit");
            return 0;
        }
        if (dot) {
            quantity /= (dot - 1);
        }
    }
    Log.info("Quantity:", quantity);

    return quantity;
}
