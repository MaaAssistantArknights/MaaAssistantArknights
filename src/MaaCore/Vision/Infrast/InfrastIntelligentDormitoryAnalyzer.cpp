#include "InfrastIntelligentDormitoryAnalyzer.h"

#include "Config/TaskData.h"
#include "InfrastSmileyImageAnalyzer.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

bool asst::InfrastIntelligentDormitoryAnalyzer::analyze()
{
    LogTraceFunction;
    m_result.clear();

    // --- 1. 寻找并排序锚点 ---
    MultiMatcher anchor_matcher(m_image);
    anchor_matcher.set_task_info("InfrastOverviewDormAnchor");

    if (!anchor_matcher.analyze() || anchor_matcher.get_result().empty()) {
        Log.info("InfrastIntelligentDormitoryAnalyzer | No anchors found");
        return false;
    }

    auto anchors = anchor_matcher.get_result();
    std::sort(anchors.begin(), anchors.end(), [](const MatchRect& a, const MatchRect& b) {
        return a.rect.y < b.rect.y;
    });

    // --- 2. 遍历锚点处理每个房间 ---
    for (const auto& match : anchors) {
        Log.info("InfrastIntelligentDormitoryAnalyzer | Analyzing room at anchor:", match.rect.to_string());
        analyze_room(match.rect);
    }

    return !m_result.empty();
}

void asst::InfrastIntelligentDormitoryAnalyzer::analyze_room(const Rect& anchor_rect)
{
    const auto& room_task_ptr = Task.get("InfrastOverviewDormRect");
    if (!room_task_ptr) {
        return;
    }
    auto room_roi = anchor_rect.move(room_task_ptr->roi);
    if ((room_roi.x + room_roi.width > m_image.cols) || (room_roi.y + room_roi.height > m_image.rows)) {
        Log.warn("InfrastIntelligentDormitoryAnalyzer | Room ROI out of image bounds:", room_roi.to_string());
        return;
    }

    infrast::InfrastDormInfo room_info;
    room_info.anchor_rect = anchor_rect;

#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw, make_rect<cv::Rect>(anchor_rect), cv::Scalar(0, 255, 0), 2);
#endif

    // 获取房间名
    const auto& name_task = Task.get<OcrTaskInfo>("InfrastOverviewDormNameRect");
    if (name_task) {
        room_info.name_rect = anchor_rect.move(name_task->rect_move);
        Log.info("IntelligentAnalyzer | Name Rect:", room_info.name_rect.to_string());
        cv::Mat name_img = m_image(make_rect<cv::Rect>(room_info.name_rect));
        RegionOCRer name_analyzer;
        name_analyzer.set_replace(name_task->replace_map, name_task->replace_full);
        name_analyzer.set_image(name_img);
        name_analyzer.set_bin_expansion(0);
        if (name_analyzer.analyze()) {
            room_info.room_name = name_analyzer.get_result().text;
            Log.info("IntelligentAnalyzer | Scanned Room Name:", room_info.room_name);
        }
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(room_info.name_rect), cv::Scalar(255, 0, 0), 2);
#endif
    }

    // 遍历 5 个槽位
    for (int i = 0; i < 5; ++i) {
        analyze_slot(i, anchor_rect, room_info);
    }

    // 特判第四个房间的第五个槽位
    if (room_info.room_name == "宿舍4" && room_info.slots_lock.size() > 4 && room_info.slots_rect.size() > 4 &&
        room_info.slots_mood.size() > 4 && room_info.slots_lock[4] == false) {
        const auto& dorm_status_task = Task.get<OcrTaskInfo>("InfrastOverviewDormStatus");
        cv::Mat dorm_status_img =
            m_image(make_rect<cv::Rect>(room_info.slots_rect[4].move(dorm_status_task->rect_move)));
        RegionOCRer dorm_status_analyzer;
        dorm_status_analyzer.set_replace(dorm_status_task->replace_map, dorm_status_task->replace_full);
        dorm_status_analyzer.set_image(dorm_status_img);
        dorm_status_analyzer.set_bin_expansion(0);
        if (dorm_status_analyzer.analyze()) {
            if (dorm_status_analyzer.get_result().text.find("休息中") != std::string::npos) {
                room_info.slots_mood[4] = 0.0;
            }
            Log.info("IntelligentAnalyzer | Scanned Room Name:", room_info.room_name);
        }
    }
    m_result.emplace_back(room_info);
}

void asst::InfrastIntelligentDormitoryAnalyzer::analyze_slot(
    int slot_idx,
    const Rect& anchor_rect,
    infrast::InfrastDormInfo& room_info)
{
    std::string task_name = "InfrastOverviewDormSlot" + std::to_string(slot_idx);
    const auto& slot_task_ptr = Task.get(task_name);
    if (!slot_task_ptr) {
        return;
    }

    auto slot_roi = anchor_rect.move(slot_task_ptr->roi);

    // 边界检查
    if ((slot_roi.x + slot_roi.width > m_image.cols) || (slot_roi.y + slot_roi.height > m_image.rows)) {
        room_info.slots_rect.emplace_back();
        room_info.slots_lock.push_back(false);
        room_info.slots_mood.push_back(-1.0);
        return;
    }

    room_info.slots_rect.push_back(slot_roi);

    // 识别是否锁定
    bool is_lock = false;
    const auto& lock_check_task_ptr = Task.get("InfrastOverviewDormSlotLock");
    if (lock_check_task_ptr) {
        Matcher lock_matcher(m_image);
        lock_matcher.set_task_info(lock_check_task_ptr);
        lock_matcher.set_roi(slot_roi);
        if (lock_matcher.analyze()) {
            is_lock = true;
        }
    }
    room_info.slots_lock.push_back(is_lock);

    // 识别心情
    double mood_ratio = -1.0;
    if (!is_lock) {
        mood_ratio = identify_smiley_and_mood(slot_roi);
    }
    room_info.slots_mood.push_back(mood_ratio);

#ifdef ASST_DEBUG
    cv::Scalar color = is_lock ? cv::Scalar(0, 255, 255) : cv::Scalar(255, 255, 0);
    cv::rectangle(m_image_draw, make_rect<cv::Rect>(slot_roi), color, 2);
    if (!is_lock) {
        std::string mood_str = mood_ratio >= 0 ? std::to_string(mood_ratio).substr(0, 4) : "Err";
        cv::putText(
            m_image_draw,
            mood_str,
            cv::Point(slot_roi.x, slot_roi.y + 20),
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,
            cv::Scalar(255, 0, 255),
            1);
    }
#endif
}

double asst::InfrastIntelligentDormitoryAnalyzer::identify_smiley_and_mood(const Rect& slot_roi)
{
    static const std::vector<std::pair<infrast::SmileyType, std::string>> smiley_tasks = {
        { infrast::SmileyType::Work, "InfrastOverviewSmileyWork" },
        { infrast::SmileyType::Rest, "InfrastOverviewSmileyRest" },
        { infrast::SmileyType::Distract, "InfrastOverviewSmileyDistract" }
    };

    double max_score = -1.0;
    std::optional<infrast::SmileyType> best_type = std::nullopt;
    Rect best_rect;

    for (const auto& [type, smiley_task_name] : smiley_tasks) {
        Matcher smiley_matcher(m_image);
        const auto& s_task_ptr = Task.get<MatchTaskInfo>(smiley_task_name);
        if (!s_task_ptr) {
            continue;
        }
        smiley_matcher.set_task_info(s_task_ptr);
        smiley_matcher.set_roi(slot_roi);
        if (smiley_matcher.analyze()) {
            const auto& res = smiley_matcher.get_result();
            if (res.score > max_score) {
                max_score = res.score;
                best_type = type;
                best_rect = res.rect;
            }
        }
    }

    if (best_type.has_value()) {
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(best_rect), cv::Scalar(0, 0, 255), 1);
        Log.debug("Best smiley match:", int(best_type.value()), "score:", max_score);
#endif
        switch (best_type.value()) {
        case infrast::SmileyType::Distract:
            return 0.0;
        case infrast::SmileyType::Rest:
            return 1.0;
        case infrast::SmileyType::Work:
            return calculate_mood_ratio(best_rect);
        case infrast::SmileyType::Invalid:
        default:
            return -1.0;
        }
    }
    return -1.0;
}

double asst::InfrastIntelligentDormitoryAnalyzer::calculate_mood_ratio(const Rect& smiley_rect)
{
    const auto prg_task_ptr = Task.get<MatchTaskInfo>("InfrastOverviewMoodProgressBar");
    if (!prg_task_ptr) {
        Log.warn("Missing Task: InfrastOverviewMoodProgressBar");
        return 0.5;
    }
    uint8_t prg_lower_limit = 160;
    int prg_diff_thres = 20;

    if (!prg_task_ptr->templ_thresholds.empty()) {
        prg_lower_limit = static_cast<uint8_t>(prg_task_ptr->templ_thresholds.front());
    }
    if (!prg_task_ptr->special_params.empty()) {
        prg_diff_thres = prg_task_ptr->special_params.front();
    }

    Rect bar_roi = smiley_rect.move(prg_task_ptr->rect_move);
    Log.info("Debug | bar_roi ROI:", bar_roi.x, bar_roi.y, bar_roi.width, bar_roi.height);
    if ((bar_roi.x + bar_roi.width > m_image.cols) || (bar_roi.y + bar_roi.height > m_image.rows)) {
        return 0.0;
    }

    cv::Mat prg_image = m_image(make_rect<cv::Rect>(bar_roi));
    cv::Mat prg_gray;
    cv::cvtColor(prg_image, prg_gray, cv::COLOR_BGR2GRAY);

    int max_white_length = 0;
    for (int i = 0; i < prg_gray.rows; ++i) {
        int cur_white_length = 0;
        uint8_t left_value = prg_lower_limit;

        for (int j = 0; j < prg_gray.cols; ++j) {
            auto value = prg_gray.at<uint8_t>(i, j);

            if (value >= prg_lower_limit && left_value < value + prg_diff_thres) {
                left_value = value;
                ++cur_white_length;
                if (max_white_length < cur_white_length) {
                    max_white_length = cur_white_length;
                }
            }
            else {
                if (max_white_length < cur_white_length) {
                    max_white_length = cur_white_length;
                }
                left_value = prg_lower_limit;
                cur_white_length = 0;
                break;
            }
        }
    }

    double ratio = static_cast<double>(max_white_length) / bar_roi.width;

#ifdef ASST_DEBUG
    cv::line(
        m_image_draw,
        cv::Point(bar_roi.x, bar_roi.y),
        cv::Point(bar_roi.x + max_white_length, bar_roi.y),
        cv::Scalar(0, 255, 0),
        2);
#endif

    return ratio;
}
