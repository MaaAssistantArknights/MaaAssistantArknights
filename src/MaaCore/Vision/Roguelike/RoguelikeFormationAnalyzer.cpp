#include "RoguelikeFormationAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/MultiMatcher.h"

MAA_VISION_NS_BEGIN

bool RoguelikeFormationAnalyzer::analyze()
{
    m_result.clear();

    MultiMatcher opers_analyzer(m_image);
    opers_analyzer.set_task_info("RoguelikeFormationOper");

    auto opers_opt = opers_analyzer.analyze();
    if (!opers_opt) {
        return false;
    }
    sort_by_vertical_(*opers_opt);
    for (const auto& oper_mr : opers_opt.value()) {
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

bool RoguelikeFormationAnalyzer::selected_analyze(const Rect& roi)
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

MAA_VISION_NS_END
