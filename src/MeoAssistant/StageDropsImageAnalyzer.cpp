#include "StageDropsImageAnalyzer.h"

#include "TaskData.h"
#include "OcrImageAnalyzer.h"
#include "MatchImageAnalyzer.h"
#include "Resource.h"
#include "AsstUtils.hpp"
#include "Logger.hpp"

bool asst::StageDropsImageAnalyzer::analyze()
{
    LogTraceFunction;

#ifdef ASST_DEBUG
    std::string stem = utils::get_format_time();
    stem = utils::string_replace_all_batch(stem, { {":", "-"}, {" ", "_"} });
    std::filesystem::create_directory("debug");
    cv::imwrite("debug/" + stem + "_raw.png", m_image);
#endif

    analyze_stage_code();
    analyze_difficulty();
    analyze_stars();
    bool ret = analyze_drops();

#ifdef ASST_DEBUG
    cv::imwrite("debug/" + stem + "_draw.png", m_image_draw);
#endif

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
    m_stage_code = analyzer.get_result().front().text;

#ifdef ASST_DEBUG
    const Rect& text_rect = analyzer.get_result().front().rect;
    cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(text_rect), cv::Scalar(0, 0, 255), 2);
    cv::putText(m_image_draw, m_stage_code, cv::Point(text_rect.x, text_rect.y - 10), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
#endif

    return true;
}

bool asst::StageDropsImageAnalyzer::analyze_stars()
{
    LogTraceFunction;

    static const std::unordered_map<int, std::string> StarsTaskName = {
        {2, "StageDrops-Stars-2"},
        {3, "StageDrops-Stars-3"},
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

#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(matched_rect), cv::Scalar(0, 0, 255), 2);
    cv::putText(m_image_draw, std::to_string(m_stars) + " stars", cv::Point(matched_rect.x, matched_rect.y + matched_rect.height + 20),
        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
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

    auto& ResrcItem = Resrc.item();
    bool has_error = false;
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
            if (item.empty() || quantity <= 0) {
                has_error = true;
                Log.error(__FUNCTION__, "error");
                continue;
            }
            StageDropInfo info;
            info.droptype = droptype;
            info.item_id = std::move(item);
            info.quantity = quantity;

            const std::string& name = ResrcItem.get_item_name(info.item_id);
            info.item_name = name.empty() ? "未知材料" : name;

            static const std::unordered_map<StageDropType, std::string> DropTypeName = {
                { StageDropType::Normal, "NORMAL_DROP" },
                { StageDropType::Extra,"EXTRA_DROP" },
                { StageDropType::Furniture, "FURNITURE" },
                { StageDropType::Special, "SPECIAL_DROP" },
                { StageDropType::ExpAndLMB, "EXP_LMB_DROP" },
                { StageDropType::Sanity, "SANITY_DROP" },
                { StageDropType::Reward, "REWARD_DROP" },
                { StageDropType::Unknown, "UNKNOWN_DROP" }
            };
            info.droptype_name = DropTypeName.at(droptype);

            m_drops.emplace_back(std::move(info));
        }
    }
    return !has_error;
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

    const int min_width = static_cast<int>(task_ptr->special_threshold);
    const int max_spacing = static_cast<int>(task_ptr->templ_threshold);

    int istart = 0, iend = bounding.cols - 1;
    bool in = true;         // 是否正处在白线中
    int spacing = 0;

    // split
    for (int i = 0; i < bounding.cols; ++i) {
        bool is_white = bounding.at<uchar>(0, i);

        if (in && !is_white) {
            in = false;
            iend = i;
            int width = iend - istart;
            if (width < min_width) {
                spacing += iend - istart;
            }
            else {
                spacing = 0;
                Rect baseline{ x_offset + istart, y_offset, width, bounding_rect.height };
                m_baseline.emplace_back(baseline, match_droptype(baseline));
            }
        }
        else if (!in && is_white) {
            istart = i;
            in = true;
        }
        else if (!in) {
            if (++spacing > max_spacing && istart != 0) {
                break;
            }
        }
    }

    if (in) {   // 最右边的白线贴着 bounding 边的情况
        int width = bounding.cols - 1 - istart;
        if (width >= min_width) {
            Rect baseline{ x_offset + istart, y_offset, width, bounding_rect.height };
            m_baseline.emplace_back(baseline, match_droptype(baseline));
        }
    }

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
        {StageDropType::Furniture, "StageDrops-DropType-Furniture"},
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
    case StageDropType::Furniture:
        return "furni";     // 家具
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
    if (!m_stage_code.empty()) {
        auto& drops = Resrc.drops().get_stage_info(m_stage_code, m_difficulty).drops;
        if (auto find_iter = drops.find(type); find_iter != drops.end()) {
            result = match_item_with_templs(find_iter->second);
        }
    }

    // 没识别到的话就把全部材料都拿来跑一遍
    if (result.empty()) {
        auto items = Resrc.item().get_all_item_id();
        result = match_item_with_templs(std::vector<std::string>(items.cbegin(), items.cend()));
        // 将这次识别到的加入该关卡的待识别列表
        if (!result.empty() && !m_stage_code.empty()) {
            Resrc.drops().append_drops(StageKey{ m_stage_code, m_difficulty }, type, result);
        }
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

    // split
    const int max_spacing = static_cast<int>(task_ptr->templ_threshold);
    std::vector<cv::Range> contours;
    int iright = bin.cols - 1, ileft = 0;
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
            ileft = i;
            in = false;
            spacing = 0;
            contours.emplace_back(ileft, iright + 1);   // range 是前闭后开的
        }
        else if (!in && has_white) {
            iright = i;
            in = true;
        }
        else if (!in) {
            if (++spacing > max_spacing &&
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
        if (dot > 1) {
            quantity /= (dot - 1);
        }
    }
    Log.info("Quantity:", quantity);

    return quantity;
}
