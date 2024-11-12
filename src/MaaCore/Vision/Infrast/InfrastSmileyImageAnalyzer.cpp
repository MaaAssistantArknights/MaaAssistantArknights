#include "InfrastSmileyImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Utils/StringMisc.hpp"
#include "Vision/MultiMatcher.h"

bool asst::InfrastSmileyImageAnalyzer::analyze()
{
    const static std::unordered_map<infrast::SmileyType, std::string> smiley_map = {
        { infrast::SmileyType::Rest, "InfrastSmileyOnRest" },
        { infrast::SmileyType::Work, "InfrastSmileyOnWork" },
        { infrast::SmileyType::Distract, "InfrastSmileyOnDistract" }
    };

    m_result.clear();

    MultiMatcher mm_analyzer(m_image);

    decltype(m_result) temp_result;
    for (const auto& [type, task_name] : smiley_map) {
        mm_analyzer.set_task_info(task_name);
        mm_analyzer.set_roi(m_roi);
        if (!mm_analyzer.analyze()) {
            continue;
        }
        auto& res = mm_analyzer.get_result();
        for (const MatchRect& mr : res) {
            temp_result.emplace_back(infrast::Smiley { type, mr.rect });
#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(mr.rect), cv::Scalar(0, 0, 255), 2);
#endif
        }
    }
    m_result = std::move(temp_result);
    return true;
}
