#include "RoguelikeFormationImageAnalyzer.h"

#include "AsstUtils.hpp"
#include "MultiMatchImageAnalyzer.h"
#include "TaskData.h"
#include "Logger.hpp"

bool asst::RoguelikeFormationImageAnalyzer::analyze()
{
    MultiMatchImageAnalyzer opers_analyzer(m_image);
    opers_analyzer.set_task_info("Roguelike1FormationOper");

    if (!opers_analyzer.analyze()) {
        return false;
    }
    opers_analyzer.sort_result();
    const auto& all_opers = opers_analyzer.get_result();
    for (const MatchRect& oper_mr : all_opers) {
        FormationOper oper;
        oper.rect = oper_mr.rect;
        oper.selected = selected_analyze(oper_mr.rect);

#ifdef ASST_DEBUG
        if (oper.selected) {
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(oper.rect), cv::Scalar(0, 0, 255), 3);
        }
        else {
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(oper.rect), cv::Scalar(0, 255, 0), 3);
        }
#endif

        m_result.emplace_back(std::move(oper));
    }

    return !m_result.empty();
}

const std::vector<asst::RoguelikeFormationImageAnalyzer::FormationOper>&
asst::RoguelikeFormationImageAnalyzer::get_result() const noexcept
{
    return m_result;
}

bool asst::RoguelikeFormationImageAnalyzer::selected_analyze(const Rect& roi)
{
    cv::Mat img_roi = m_image(utils::make_rect<cv::Rect>(roi));
    cv::Mat hsv;

    const auto selected_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        Task.get("Roguelike1FormationOperSelected"));
    cv::cvtColor(img_roi, hsv, cv::COLOR_BGR2HSV);
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
    Log.trace("selected_analyze |", count);

    return count >= selected_task_ptr->templ_threshold;
}
