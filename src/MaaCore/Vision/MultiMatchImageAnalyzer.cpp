#include "MultiMatchImageAnalyzer.h"

#include "Utils/Ranges.hpp"
#include <utility>

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"

asst::MultiMatchImageAnalyzer::MultiMatchImageAnalyzer(const cv::Mat& image, std::string templ_name, double templ_thres)
    : AbstractImageAnalyzer(image), m_templ_name(std::move(templ_name)), m_templ_thres(templ_thres)
{}

bool asst::MultiMatchImageAnalyzer::analyze()
{
    if (m_log_tracing) {
        Log.trace("MultiMatchImageAnalyzer::analyze | ", m_templ_name);
    }
    m_result.clear();

    const cv::Mat templ = TemplResource::get_instance().get_templ(m_templ_name);
    if (templ.empty()) {
        Log.error("templ is empty!", m_templ_name);
#ifdef ASST_DEBUG
        throw std::runtime_error("templ is empty: " + m_templ_name);
#endif
        return false;
    }

    return multi_match_templ(templ);
}

void asst::MultiMatchImageAnalyzer::sort_result_horizontal()
{
    // 按位置排个序
    ranges::sort(m_result, [](const MatchRect& lhs, const MatchRect& rhs) -> bool {
        if (std::abs(lhs.rect.y - rhs.rect.y) < 5) { // y差距较小则理解为是同一排的，按x排序
            return lhs.rect.x < rhs.rect.x;
        }
        else {
            return lhs.rect.y < rhs.rect.y;
        }
    });
}

void asst::MultiMatchImageAnalyzer::sort_result_vertical()
{
    // 按位置排个序
    ranges::sort(m_result, [](const MatchRect& lhs, const MatchRect& rhs) -> bool {
        if (std::abs(lhs.rect.x - rhs.rect.x) < 5) { // x差距较小则理解为是同一排的，按y排序
            return lhs.rect.y < rhs.rect.y;
        }
        else {
            return lhs.rect.x < rhs.rect.x;
        }
    });
}

void asst::MultiMatchImageAnalyzer::set_mask_range(int lower, int upper) noexcept
{
    m_mask_range = std::make_pair(lower, upper);
}

void asst::MultiMatchImageAnalyzer::set_mask_range(std::pair<int, int> mask_range) noexcept
{
    m_mask_range = std::move(mask_range);
}

void asst::MultiMatchImageAnalyzer::set_templ_name(std::string templ_name) noexcept
{
    m_templ_name = std::move(templ_name);
}

void asst::MultiMatchImageAnalyzer::set_threshold(double templ_thres) noexcept
{
    m_templ_thres = templ_thres;
}

void asst::MultiMatchImageAnalyzer::set_task_info(MatchTaskInfo task_info) noexcept
{
    m_mask_range = std::move(task_info.mask_range);
    m_templ_name = std::move(task_info.templ_name);
    m_templ_thres = task_info.templ_threshold;
    set_roi(task_info.roi);
}

void asst::MultiMatchImageAnalyzer::set_task_info(const std::shared_ptr<TaskInfo>& task_ptr)
{
    set_task_info(*std::dynamic_pointer_cast<MatchTaskInfo>(task_ptr));
}

void asst::MultiMatchImageAnalyzer::set_task_info(const std::string& task_name)
{
    set_task_info(Task.get(task_name));
}

const std::vector<asst::MatchRect>& asst::MultiMatchImageAnalyzer::get_result() const noexcept
{
    return m_result;
}

bool asst::MultiMatchImageAnalyzer::multi_match_templ(const cv::Mat templ)
{
    cv::Mat matched;
    cv::Mat image_roi = m_image(make_rect<cv::Rect>(m_roi));

    if (templ.cols > image_roi.cols || templ.rows > image_roi.rows) {
        Log.error("templ size is too large", m_templ_name, "image_roi size:", image_roi.cols, image_roi.rows,
                  "templ size:", templ.cols, templ.rows);
        return false;
    }

    if (m_mask_range.first == 0 && m_mask_range.second == 0) {
        cv::matchTemplate(image_roi, templ, matched, cv::TM_CCOEFF_NORMED);
    }
    else {
        cv::Mat mask;
        cv::cvtColor(templ, mask, cv::COLOR_BGR2GRAY);
        // cv::threshold(mask, mask, m_mask_range.first, 255, cv::THRESH_BINARY);
        cv::inRange(mask, m_mask_range.first, m_mask_range.second, mask);
        cv::matchTemplate(image_roi, templ, matched, cv::TM_CCOEFF_NORMED, mask);
    }

    int mini_distance = (std::min)(templ.cols, templ.rows) / 2;
    for (int i = 0; i != matched.rows; ++i) {
        for (int j = 0; j != matched.cols; ++j) {
            auto value = matched.at<float>(i, j);
            if (m_templ_thres <= value && value < 2.0) {
                Rect rect(j + m_roi.x, i + m_roi.y, templ.cols, templ.rows);
                bool need_push = true;
                // 如果有两个点离得太近，只取里面得分高的那个
                // 一般相邻的都是刚刚push进去的，这里倒序快一点
                for (auto& iter : ranges::reverse_view(m_result)) {
                    if (std::abs(j + m_roi.x - iter.rect.x) < mini_distance &&
                        std::abs(i + m_roi.y - iter.rect.y) < mini_distance) {
                        if (iter.score < value) {
                            iter.rect = rect;
                            iter.score = value;
                        } // else 这个点就放弃了
                        need_push = false;
                        break;
                    }
                }
                if (need_push) {
                    m_result.emplace_back(value, rect);
                }
            }
        }
    }

#ifdef ASST_DEBUG
    for (const auto& rect : m_result) {
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(rect.rect), cv::Scalar(0, 0, 255), 2);
        cv::putText(m_image_draw, std::to_string(rect.score), cv::Point(rect.rect.x, rect.rect.y), 1, 1,
                    cv::Scalar(0, 0, 255));
    }
#endif

    if (m_log_tracing) {
        Log.trace("multi_match_templ | ", m_templ_name, "result:", m_result, "roi:", m_roi);
    }

    return !m_result.empty();
}
