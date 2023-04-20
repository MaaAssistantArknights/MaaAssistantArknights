#include "RoguelikeFormationImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/MultiMatchImageAnalyzer.h"
#include "Vision/OcrWithFlagTemplImageAnalyzer.h"

bool asst::RoguelikeFormationImageAnalyzer::analyze()
{
    m_result.clear();

    OcrWithFlagTemplImageAnalyzer analyzer(m_image);
    analyzer.set_task_info("RoguelikeFormationOper", "RoguelikeFormationOcr");
    analyzer.set_replace(Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map,
                         Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_full);
    analyzer.set_threshold(Task.get("RoguelikeFormationOcr")->specific_rect.x);

    if (!analyzer.analyze()) {
        return false;
    }

    analyzer.sort_result_vertical();
    auto& rect_list = analyzer.get_flag_result();
    ranges::sort(rect_list, [](const Rect& lhs, const Rect& rhs) -> bool {
        if (std::abs(lhs.x - rhs.x) < 5) {
            return lhs.y < rhs.y;
        }
        else {
            return lhs.x < rhs.x;
        }
    });
    int pos = 0;
    for (const auto& [_, text_rect, name] : analyzer.get_result()) {
        Rect rect = rect_list[pos];
        FormationOper oper;
        oper.rect = rect;
        oper.selected = selected_analyze(rect);
        oper.name = name;
        pos++;

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
