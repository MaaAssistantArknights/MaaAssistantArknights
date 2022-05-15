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

    analyze_stage_name();
    analyze_difficulty();
    analyze_drops();

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

#ifdef ASST_DEBUG
    m_difficulty = StageDifficulty::Normal;
    return true;
#endif // ASST_DEBUG
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
            Log.info("item", item);
#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(item_roi), cv::Scalar(0, 0, 255), 2);
            cv::putText(m_image_draw, item, cv::Point(item_roi.x, item_roi.y - 10), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
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
    for (int i = 0; i < bounding.cols; ++i) {
        bool is_white = static_cast<bool>(bounding.at<uchar>(0, i));

        if (in && !is_white) {
            iend = i;
            in = false;
            Rect baseline{ x_offset + istart, y_offset, iend - istart, bounding_rect.height };
            m_baseline.emplace_back(baseline, match_droptype(baseline));
        }
        else if (!in && is_white) {
            istart = i;
            in = true;
        }
    }
    if (in) {
        Rect baseline{ x_offset + istart, y_offset, bounding.cols - 1 - istart, bounding_rect.height };
        m_baseline.emplace_back(baseline, match_droptype(baseline));
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
        //{StageDropType::Funriture, "StageDrops-DropType-Funriture"},
        //{StageDropType::Special, "StageDrops-DropType-Special"},
    };

    MatchImageAnalyzer analyzer(m_image);
    StageDropType matched = StageDropType::Normal;
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
    cv::putText(m_image_draw, matched_name, cv::Point(matched_roi.x, matched_roi.y + matched_roi.height + 30),
        cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
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
        return "funriture"; // 家具
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

    std::vector<std::string> templs;
    if (!m_stage_name.empty()) {
        templs = Resrc.drops().get_stage_info(m_stage_name, m_difficulty).drops.at(type);
    }
    std::string result = match_item_with_templs(templs);

    if (result.empty()) {
        auto items = Resrc.item().get_all_item_id();
        templs.assign(items.cbegin(), items.cend());
        result = match_item_with_templs(templs);
    }

    return result;
}
