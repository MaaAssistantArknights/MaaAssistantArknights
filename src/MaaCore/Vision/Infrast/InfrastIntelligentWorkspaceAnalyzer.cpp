#include "InfrastIntelligentWorkspaceAnalyzer.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"
#include "InfrastSmileyImageAnalyzer.h"
#include "Utils/NoWarningCV.h"

bool asst::InfrastIntelligentWorkspaceAnalyzer::analyze(bool is_end)
{
    LogTraceFunction;
    m_result.clear();

    // --- 1. 寻找并排序锚点 ---
    MultiMatcher anchor_matcher(m_image);
    anchor_matcher.set_task_info("InfrastOverviewAnchor");

    if (!anchor_matcher.analyze() || anchor_matcher.get_result().empty()) {
        Log.info("InfrastIntelligentWorkspaceAnalyzer | No anchors found");
        return false;
    }

    auto anchors = anchor_matcher.get_result();
    std::sort(anchors.begin(), anchors.end(), [](const MatchRect& a, const MatchRect& b) {
        return a.rect.y < b.rect.y;
    });

    if(!is_end) {
        // --- 2. 遍历锚点处理每个房间 ---
        for (const auto& match : anchors) {
            analyze_room(match.rect);
        }
    }

    if (is_end) {
        // --- 3. 处理最后没有anchor的房间 ---
        int row_pitch = 200; 
        if (anchors.size() >= 2) {
            row_pitch = (anchors.back().rect.y - anchors.front().rect.y) / static_cast<int>(anchors.size() - 1);
        }
        Rect last_rect = anchors.back().rect;
        Rect virtual_rect1 = last_rect;
        virtual_rect1.y += row_pitch;
        if (virtual_rect1.y < m_image.rows) {
            Log.info("IntelligentAnalyzer | Scanning Virtual Room 1:", virtual_rect1.to_string());
            analyze_room(virtual_rect1);
        }

        Rect virtual_rect2 = virtual_rect1;
        virtual_rect2.y += row_pitch;
        if (virtual_rect2.y < m_image.rows) {
            Log.info("IntelligentAnalyzer | Scanning Virtual Room 2:", virtual_rect2.to_string());
            analyze_room(virtual_rect2);
        }
    }

    return !m_result.empty();
}

void asst::InfrastIntelligentWorkspaceAnalyzer::analyze_room(const Rect& anchor_rect)
{
    infrast::InfrastRoomInfo room_info;
    room_info.anchor_rect = anchor_rect;

#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw, make_rect<cv::Rect>(anchor_rect), cv::Scalar(0, 255, 0), 2);
#endif

// --- 1. 获取房间名切图并识别 ---
    const auto& name_task = Task.get<OcrTaskInfo>("InfrastOverviewNameRect");
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
        // 画框和文字调试
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(room_info.name_rect), cv::Scalar(255, 0, 0), 2);
#endif
    }
    // 训练室特判
    if (room_info.room_name.find("训练室") != std::string::npos) {
        const auto& training_status_task = Task.get<OcrTaskInfo>("InfrastOverviewTrainingStatus");
        Rect training_status_rect = anchor_rect.move(training_status_task->rect_move);
        Log.info("IntelligentAnalyzer | Training Status Rect:", training_status_rect.to_string());
        cv::Mat status_img = m_image(make_rect<cv::Rect>(training_status_rect));
        RegionOCRer status_analyzer;
        status_analyzer.set_replace(training_status_task->replace_map, training_status_task->replace_full);
        status_analyzer.set_image(status_img);
        status_analyzer.set_bin_expansion(0);
        if (status_analyzer.analyze()) {
            std::string status_text = status_analyzer.get_result().text;
            if (status_text.find("训练中") != std::string::npos) {
                Log.info("IntelligentAnalyzer | Room is confirmed: In Training.");
                room_info.is_training = true;
            }
        }
    }
    // 2 遍历 5 个槽位
    for (int i = 0; i < 5; ++i) {
        analyze_slot(i, anchor_rect, room_info);
    }

    m_result.emplace_back(room_info);
}

void asst::InfrastIntelligentWorkspaceAnalyzer::analyze_slot(int slot_idx, const Rect& anchor_rect, infrast::InfrastRoomInfo& room_info)
{
    std::string task_name = "InfrastOverview-Slot-" + std::to_string(slot_idx);
    const auto& slot_task_ptr = Task.get(task_name);
    if (!slot_task_ptr) return;

    auto slot_roi = anchor_rect.move(slot_task_ptr->roi);

    // 边界检查
    if ((slot_roi.x + slot_roi.width > m_image.cols) || (slot_roi.y + slot_roi.height > m_image.rows)) {
        room_info.slots_rect.emplace_back();
        room_info.slots_empty.push_back(false);
        room_info.slots_mood.push_back(-1.0);
        return;
    }

    room_info.slots_rect.push_back(slot_roi);

    // 识别是否为空位
    bool is_empty = false;
    const auto& empty_check_task_ptr = Task.get("InfrastOverviewSlotEmpty");
    if (empty_check_task_ptr) {
        Matcher empty_matcher(m_image);
        empty_matcher.set_task_info(empty_check_task_ptr);
        empty_matcher.set_roi(slot_roi);
        if (empty_matcher.analyze()) {
            is_empty = true;
        }
    }
    room_info.slots_empty.push_back(is_empty);

    // 识别心情
    double mood_ratio = -1.0;
    if (!is_empty) {
        mood_ratio = identify_smiley_and_mood(slot_roi);
    }
    room_info.slots_mood.push_back(mood_ratio);

#ifdef ASST_DEBUG
    cv::Scalar color = is_empty ? cv::Scalar(0, 255, 255) : cv::Scalar(255, 255, 0);
    cv::rectangle(m_image_draw, make_rect<cv::Rect>(slot_roi), color, 2);
    if (!is_empty) {
        std::string mood_str = mood_ratio >= 0 ? std::to_string(mood_ratio).substr(0, 4) : "Err";
        cv::putText(m_image_draw, mood_str, cv::Point(slot_roi.x, slot_roi.y + 20), 
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 255), 1);
    }
#endif
}

double asst::InfrastIntelligentWorkspaceAnalyzer::identify_smiley_and_mood(const Rect& slot_roi)
{
    static const std::vector<std::pair<infrast::SmileyType, std::string>> smiley_tasks = {
        { infrast::SmileyType::Work,     "InfrastOverviewSmileyWork" },
        { infrast::SmileyType::Rest,     "InfrastOverviewSmileyRest" },
        { infrast::SmileyType::Distract, "InfrastOverviewSmileyDistract" }
    };

    for (const auto& [type, smiley_task_name] : smiley_tasks) {
        Matcher smiley_matcher(m_image);
        const auto& s_task_ptr = Task.get<MatchTaskInfo>(smiley_task_name);
        if (!s_task_ptr) continue;

        smiley_matcher.set_task_info(s_task_ptr);
        smiley_matcher.set_roi(slot_roi);

        if (smiley_matcher.analyze()) {
            const auto& res = smiley_matcher.get_result();
#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(res.rect), cv::Scalar(0, 0, 255), 1);
#endif
            switch (type) {
                case infrast::SmileyType::Distract: return 0.0;
                case infrast::SmileyType::Rest:     return 1.0;
                case infrast::SmileyType::Work:     return calculate_mood_ratio(res.rect);
            }
        }
    }
    return -1.0;
}

double asst::InfrastIntelligentWorkspaceAnalyzer::calculate_mood_ratio(const Rect& smiley_rect)
{
    // 需要在 tasks.json 中定义这个任务，配置 relative roi (rect_move)
    const auto prg_task_ptr = Task.get<MatchTaskInfo>("InfrastOverviewMoodProgressBar");
    if (!prg_task_ptr) {
        Log.warn("Missing Task: InfrastOverviewMoodProgressBar");
        return 0.5; // fallback
    }
    // 参数读取 (灰度阈值，跳变阈值)
    uint8_t prg_lower_limit = 160; // 默认值
    int prg_diff_thres = 20;       // 默认值
   
    // 如果配置了 specialParams 则覆盖默认值 (模仿原代码)
    if (!prg_task_ptr->templ_thresholds.empty()) {
        prg_lower_limit = static_cast<uint8_t>(prg_task_ptr->templ_thresholds.front());
    }
    if (!prg_task_ptr->special_params.empty()) {
        prg_diff_thres = prg_task_ptr->special_params.front();
    }

    // 计算进度条的绝对 ROI (基于笑脸位置偏移)
    // 注意：InfrastOverview 界面的布局可能和进驻界面不同，需要重新测量 rect_move
    Rect bar_roi = smiley_rect.move(prg_task_ptr->rect_move);
    Log.info("Debug | bar_roi ROI:", bar_roi.x, bar_roi.y, bar_roi.width, bar_roi.height);
    // 边界保护
    if ((bar_roi.x + bar_roi.width > m_image.cols) ||
        (bar_roi.y + bar_roi.height > m_image.rows)) {
        return 0.0;
    }

    // 图像处理：切图 -> 转灰度
    cv::Mat prg_image = m_image(make_rect<cv::Rect>(bar_roi));
    cv::Mat prg_gray;
    cv::cvtColor(prg_image, prg_gray, cv::COLOR_BGR2GRAY);

    int max_white_length = 0;

    // 扫描白色像素长度 (原汁原味的算法)
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
            } else {
                if (max_white_length < cur_white_length) {
                    max_white_length = cur_white_length;
                }
                left_value = prg_lower_limit;
                cur_white_length = 0;
                // 遇到黑点直接中断这一行的扫描? 原代码是 break，我们保持一致
                break;
            }
        }
    }

    double ratio = static_cast<double>(max_white_length) / bar_roi.width;

#ifdef ASST_DEBUG
    // 在进度条位置画线，方便调试 rect_move 是否对齐
    cv::line(m_image_draw, cv::Point(bar_roi.x, bar_roi.y),
             cv::Point(bar_roi.x + max_white_length, bar_roi.y), cv::Scalar(0, 255, 0), 2);
#endif

    return ratio;
}