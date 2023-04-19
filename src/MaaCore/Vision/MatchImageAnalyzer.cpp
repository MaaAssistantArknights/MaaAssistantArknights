#include "MatchImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

const asst::MatchImageAnalyzer::ResultOpt& asst::MatchImageAnalyzer::analyze()
{
    m_result = std::nullopt;

    const auto& [matched, templ, templ_name] = preproc_and_match();

    if (matched.empty()) {
        return std::nullopt;
    }

    double min_val = 0.0, max_val = 0.0;
    cv::Point min_loc, max_loc;
    cv::minMaxLoc(matched, &min_val, &max_val, &min_loc, &max_loc);

    Rect rect(max_loc.x + m_roi.x, max_loc.y + m_roi.y, templ.cols, templ.rows);
    if (max_val > 2.0) {
        max_val = 0;
    }
    if (m_log_tracing && max_val > m_templ_thres * 0.7) { // 得分太低的肯定不对，没必要打印
        Log.trace("match_templ |", templ_name, "score:", max_val, "rect:", rect, "roi:", m_roi);
    }

    if (m_templ_thres <= max_val) {
        m_result = Result { .rect = rect, .score = max_val };
    }
    return m_result;
}

void asst::MatchImageAnalyzer::set_mask_range(int lower, int upper, bool mask_with_src) noexcept
{
    m_mask_range = std::make_pair(lower, upper);
    m_mask_with_src = mask_with_src;
}

void asst::MatchImageAnalyzer::set_templ(std::variant<std::string, cv::Mat> templ)
{
    m_templ = std::move(templ);
}

void asst::MatchImageAnalyzer::set_threshold(double templ_thres) noexcept
{
    m_templ_thres = templ_thres;
}

void asst::MatchImageAnalyzer::set_task_info(const std::shared_ptr<TaskInfo>& task_ptr)
{
    set_task_info(*std::dynamic_pointer_cast<MatchTaskInfo>(task_ptr));
}

void asst::MatchImageAnalyzer::set_task_info(const std::string& task_name)
{
    set_task_info(Task.get(task_name));
}

void asst::MatchImageAnalyzer::set_mask_with_close(int with_close) noexcept
{
    m_mask_with_close = with_close;
}

void asst::MatchImageAnalyzer::set_task_info(MatchTaskInfo task_info) noexcept
{
    m_templ = std::move(task_info.templ_name);
    m_templ_thres = task_info.templ_threshold;
    m_mask_range = std::move(task_info.mask_range);

    set_roi(task_info.roi);
}

asst::MatchImageAnalyzer::RawResult asst::MatchImageAnalyzer::preproc_and_match()
{
    cv::Mat templ;
    std::string templ_name;

    if (std::holds_alternative<std::string>(m_templ)) {
        templ_name = std::get<std::string>(m_templ);
        templ = TemplResource::get_instance().get_templ(templ_name);
    }
    else if (std::holds_alternative<cv::Mat>(m_templ)) {
        templ = std::get<cv::Mat>(m_templ);
    }
    else {
        Log.error("templ is none");
    }

    if (templ.empty()) {
        Log.error("templ is empty!", templ_name);
#ifdef ASST_DEBUG
        throw std::runtime_error("templ is empty: " + templ_name);
#endif
        return {};
    }

    cv::Mat image_roi = make_roi(m_image, m_roi);
    if (templ.cols > image_roi.cols || templ.rows > image_roi.rows) {
        Log.error("templ size is too large", templ_name, "image_roi size:", image_roi.cols, image_roi.rows,
                  "templ size:", templ.cols, templ.rows);
        return {};
    }

    cv::Mat matched;
    if (m_mask_range.first == 0 && m_mask_range.second == 0) {
        cv::matchTemplate(image_roi, templ, matched, cv::TM_CCOEFF_NORMED);
    }
    else {
        cv::Mat mask;
        cv::cvtColor(m_mask_with_src ? image_roi : templ, mask, cv::COLOR_BGR2GRAY);
        cv::inRange(mask, m_mask_range.first, m_mask_range.second, mask);
        if (m_mask_with_close) {
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
            cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
        }
        cv::matchTemplate(image_roi, templ, matched, cv::TM_CCOEFF_NORMED, mask);
    }

    return RawResult { .matched = matched, .templ = templ, .templ_name = templ_name };
}

const asst::MatchImageAnalyzer::ResultOpt& asst::MatchImageAnalyzer::result() const noexcept
{
    return m_result;
}
