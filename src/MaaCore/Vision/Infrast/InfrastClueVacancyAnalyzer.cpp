#include "InfrastClueVacancyAnalyzer.h"

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Vision/Matcher.h"

MAA_VISION_NS_BEGIN

bool InfrastClueVacancyAnalyzer::analyze()
{
    const static std::string clue_vacancy = "InfrastClueVacancy";

    Matcher analyzer(m_image);

    for (const std::string& suffix : m_to_be_analyzed) {
        analyzer.set_task_info(clue_vacancy + suffix);
        auto result_opt = analyzer.analyze();
        if (!result_opt) {
            Log.trace("no", clue_vacancy, suffix);
            continue;
        }
        Rect rect = result_opt->rect;
        Log.trace("has", clue_vacancy, suffix);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(rect), cv::Scalar(0, 0, 255), 2);
        cv::putText(m_image_draw, suffix, cv::Point(rect.x, rect.y + 1), 0, 1, cv::Scalar(0, 0, 255), 2);
#endif
        m_clue_vacancy.emplace(suffix, rect);
    }

    return !m_clue_vacancy.empty();
}

MAA_VISION_NS_END
