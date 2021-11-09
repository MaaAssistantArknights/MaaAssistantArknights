#include "InfrastMoodImageAnalyzer.h"

#include "AsstUtils.hpp"
#include "InfrastSmileyImageAnalyzer.h"
#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "Resource.h"

bool asst::InfrastMoodImageAnalyzer::analyze()
{
    LogTraceFunction;

    mood_detect();
    mood_analyze();
    hash_calc();
    selected_analyze();
    working_analyze();

    for (auto&& info : m_result) {
        log.trace(info.hash, info.rect.to_string(),
                  "smiley:", (int)info.smiley.type, "mood:", info.percentage,
                  "selected:", info.selected, "working:", info.working);
    }

    return true;
}

void asst::InfrastMoodImageAnalyzer::sort_result()
{
    std::sort(m_result.begin(), m_result.end(),
              [](const InfrastOperMoodInfo& lhs, const InfrastOperMoodInfo& rhs) -> bool {
                  // 先按心情排序，心情低的放前面
                  if (std::fabs(lhs.percentage - rhs.percentage) > DoubleDiff) {
                      return lhs.percentage < rhs.percentage;
                  }
                  // 心情一样的就按位置排序，左边的放前面
                  if (std::abs(lhs.rect.x - rhs.rect.x) > 5) {
                      return lhs.rect.x < rhs.rect.x;
                  }
                  else {
                      return lhs.rect.y < rhs.rect.y;
                  }
              });
}

bool asst::InfrastMoodImageAnalyzer::mood_detect()
{
    const auto upper_task_ptr = resource.task().task_ptr("InfrastSkillsUpper");
    const auto lower_task_ptr = resource.task().task_ptr("InfrastSkillsLower");
    const auto prg_task_ptr = resource.task().task_ptr("InfrastOperMoodProgressBar");

    Rect progress_rect_move = prg_task_ptr->rect_move;

    std::vector<Rect> roi_vec = {
        upper_task_ptr->roi,
        lower_task_ptr->roi
    };

    InfrastSmileyImageAnalyzer smiley_analyzer(m_image);

    for (auto&& roi : roi_vec) {
        smiley_analyzer.set_roi(roi);
        if (!smiley_analyzer.analyze()) {
            return false;
        }

        const auto& smiley_res = smiley_analyzer.get_result();

        for (const auto& smiley : smiley_res) {
            Rect prg_rect = progress_rect_move;
            prg_rect.x += smiley.rect.x;
            prg_rect.y += smiley.rect.y;
            // mood_analyze是可以识别长度不够的心情条的
            // 这里主要是为了算立绘hash，宽度不够的hash不好算，直接忽略了
            if (prg_rect.x + prg_rect.width >= roi.x + roi.width || prg_rect.x < roi.x) {
                continue;
            }
#ifdef LOG_TRACE
            //cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(prg_rect), cv::Scalar(0, 0, 255), 2);
#endif // LOG_TRACE

            InfrastOperMoodInfo info;
            info.smiley = smiley;
            info.rect = prg_rect;
            m_result.emplace_back(std::move(info));
        }
    }
}

bool asst::InfrastMoodImageAnalyzer::mood_analyze()
{
    const auto prg_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("InfrastOperMoodProgressBar"));
    int prg_lower_limit = prg_task_ptr->templ_threshold;
    int prg_diff_thres = prg_task_ptr->hist_threshold;

    for (auto iter = m_result.begin(); iter != m_result.end();) {
        bool not_analyze = false;
        switch (iter->smiley.type) {
        case InfrastSmileyType::Distract:
            iter->percentage = 0;
            not_analyze = true;
            break;
        case InfrastSmileyType::Rest:
            iter->percentage = 1.0;
            not_analyze = true;
            break;
        case InfrastSmileyType::Work:
            not_analyze = false;
            break;
        default:
            // TODO 报错
            break;
        }

        if (not_analyze) {
            ++iter;
            continue;
        }

        Rect roi = iter->rect;
        if (roi.x + roi.width >= m_image.cols) {
            roi.width = m_image.cols - roi.x;
        }
        cv::Mat prg_image = m_image(utils::make_rect<cv::Rect>(roi));
        cv::Mat prg_gray;
        cv::cvtColor(prg_image, prg_gray, cv::COLOR_BGR2GRAY);

        int max_white_length = 0; // 最长横扫的白色长度，即作为进度条长度
        for (int i = 0; i != prg_gray.rows; ++i) {
            int cur_white_length = 0;
            cv::uint8_t left_value = prg_lower_limit;
            for (int j = 0; j != prg_gray.cols; ++j) {
                auto value = prg_gray.at<cv::uint8_t>(i, j);
                // 当前点的颜色，需要大于最低阈值；且与相邻点的差值不能过大，否则就认为当前点不是进度条
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
        // 如果进度条的长度等于ROI的宽度，则说明这个进度条不完整，可能图像再往右滑，还有一部分进度条
        // 所以忽略掉这个结果
        if (roi.width != iter->rect.width && max_white_length == prg_gray.cols) {
            iter = m_result.erase(iter);
            continue;
        }

        // TODO：这里的进度条长度算的并不是特别准，属于能跑就行。有空再优化下
        double pct = static_cast<double>(max_white_length) / iter->rect.width;
        iter->percentage = pct;
#ifdef LOG_TRACE
        cv::Point p1(roi.x, roi.y);
        cv::Point p2(roi.x + max_white_length, roi.y);
        cv::line(m_image_draw, p1, p2, cv::Scalar(0, 255, 0), 1);
        cv::putText(m_image_draw, std::to_string(pct), p1, 1, 1.0, cv::Scalar(0, 255, 0));
#endif // LOG_TRACE
        ++iter;
    }

    return false;
}

bool asst::InfrastMoodImageAnalyzer::hash_calc()
{
    const auto hash_task_ptr = resource.task().task_ptr("InfrastSkillsHash");
    Rect hash_rect_move = hash_task_ptr->rect_move;

    for (auto&& info : m_result) {
        Rect hash_rect = hash_rect_move;
        hash_rect.x += info.smiley.rect.x;
        hash_rect.y += info.smiley.rect.y;
        info.hash = calc_hash(hash_rect);
    }
    return true;
}

bool asst::InfrastMoodImageAnalyzer::selected_analyze()
{
    const auto selected_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("InfrastOperSelected"));
    Rect rect_move = selected_task_ptr->rect_move;

    for (auto&& info : m_result) {
        cv::Rect selected_rect = utils::make_rect<cv::Rect>(rect_move);
        selected_rect.x += info.smiley.rect.x;
        selected_rect.y += info.smiley.rect.y;

        cv::Mat roi = m_image(selected_rect);
        cv::Mat hsv, bin;
        cv::cvtColor(roi, hsv, cv::COLOR_BGR2HSV);
        std::vector<cv::Mat> channels;
        cv::split(hsv, channels);
        int mask_lowb = selected_task_ptr->mask_range.first;
        int mask_uppb = selected_task_ptr->mask_range.second;

        int count = 0;
        auto& h_channel = channels.at(0);
        for (int i = 0; i != h_channel.rows; ++i) {
            for (int j = 0; j != h_channel.cols; ++j) {
                cv::uint8_t value = h_channel.at<cv::uint8_t>(i, j);
                if (mask_lowb < value && value < mask_uppb) {
                    ++count;
                }
            }
        }
        log.trace("selected_analyze |", count);
        info.selected = count >= selected_task_ptr->templ_threshold;
    }
    return true;
}

bool asst::InfrastMoodImageAnalyzer::working_analyze()
{
    const auto working_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("InfrastOperOnShift"));
    Rect rect_move = working_task_ptr->rect_move;

    MatchImageAnalyzer working_analyzer(m_image);
    working_analyzer.set_task_info(*working_task_ptr);

    for (auto&& info : m_result) {
        Rect working_rect = rect_move;
        working_rect.x += info.smiley.rect.x;
        working_rect.y += info.smiley.rect.y;
        working_analyzer.set_roi(working_rect);
        if (working_analyzer.analyze()) {
            info.working = true;
#ifdef LOG_TRACE
            cv::putText(m_image_draw, "ONSHIFT", cv::Point(working_rect.x, working_rect.y), 1, 1, cv::Scalar(0, 0, 255), 2);
#endif
        }
    }
    return true;
}
