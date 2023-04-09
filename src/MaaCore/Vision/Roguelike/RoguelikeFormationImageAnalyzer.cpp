#include "RoguelikeFormationImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/MultiMatchImageAnalyzer.h"

bool asst::RoguelikeFormationImageAnalyzer::analyze()
{
    m_result.clear();

    MultiMatchImageAnalyzer opers_analyzer(m_image);
    opers_analyzer.set_task_info("RoguelikeFormationOper");

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
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(oper.rect), cv::Scalar(0, 0, 255), 3);
        }
        else {
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(oper.rect), cv::Scalar(0, 255, 0), 3);
        }
#endif

        m_result.emplace_back(oper);
    }

    return !m_result.empty();
}

const std::vector<asst::RoguelikeFormationImageAnalyzer::FormationOper>& asst::RoguelikeFormationImageAnalyzer::
    get_result() const noexcept
{
    return m_result;
}

bool asst::RoguelikeFormationImageAnalyzer::selected_analyze(const Rect& roi)
{
    cv::Mat img_roi = m_image(make_rect<cv::Rect>(roi));
    cv::Mat bin;
    cv::inRange(img_roi, cv::Scalar::all(240), cv::Scalar::all(255), bin);

    // If selected, all the white pixels would be masked by blue stuff, leaving only white digits drawn on top of the
    // mask. Thus, we count and compare white pixels in upper half (where the digits were) and lower half.
    int upper = cv::countNonZero(bin(cv::Rect(bin.cols / 4, 0, bin.cols / 2, bin.rows / 2)));
    int lower = cv::countNonZero(bin(cv::Rect(bin.cols / 4, bin.rows / 2, bin.cols / 2, bin.rows / 2)));
    Log.trace("selected_analyze |", upper, ':', lower);

    return upper > 250 && lower < 2;
}
