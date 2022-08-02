#include "RoguelikeFormationImageAnalyzer.h"

#include "NoWarningCV.h"

#include "AsstUtils.hpp"
#include "MultiMatchImageAnalyzer.h"
#include "TaskData.h"
#include "Logger.hpp"

bool asst::RoguelikeFormationImageAnalyzer::analyze()
{
    m_result.clear();

    MultiMatchImageAnalyzer opers_analyzer(m_image);
    opers_analyzer.set_task_info("Roguelike1FormationOper");

    if (!opers_analyzer.analyze()) {
        return false;
    }
    opers_analyzer.sort_result_vertical();
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

        m_result.emplace_back(oper);
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
    cv::cvtColor(img_roi, hsv, cv::COLOR_BGR2HSV);

    const auto selected_task_ptr = Task.get<MatchTaskInfo>("Roguelike1FormationOperSelected");
    int h_low = selected_task_ptr->mask_range.first;
    int h_up = selected_task_ptr->mask_range.second;
    int s_low = selected_task_ptr->specific_rect.x;
    int s_up = selected_task_ptr->specific_rect.y;
    int v_low = selected_task_ptr->specific_rect.width;
    int v_up = selected_task_ptr->specific_rect.height;

    cv::Mat bin;
    cv::inRange(hsv, cv::Scalar(h_low, s_low, v_low), cv::Scalar(h_up, s_up, v_up), bin);

    int count = 0;
    for (int i = 0; i != bin.rows; ++i) {
        for (int j = 0; j != bin.cols; ++j) {
            cv::uint8_t value = bin.at<cv::uint8_t>(i, j);
            if (value) {
                ++count;
            }
        }
    }
    Log.trace("selected_analyze |", count);

    return count >= selected_task_ptr->templ_threshold;
}
