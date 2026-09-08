#include "InfrastSmileyImageAnalyzer.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Vision/MultiMatcher.h"

bool asst::InfrastSmileyImageAnalyzer::analyze()
{
    const static std::unordered_map<std::string, infrast::SmileyType> templ_name_to_smiley_type = {
        { "SmileyOnRest.png", infrast::SmileyType::Rest },
        { "SmileyOnWork.png", infrast::SmileyType::Work },
        { "SmileyOnDistract.png", infrast::SmileyType::Distract }
    };

    m_result.clear();

    MultiMatcher analyzer(m_image);
    analyzer.set_task_info("InfrastSmiley");
    analyzer.set_roi(m_roi);

    if (!analyzer.analyze()) {
        return false;
    }

    std::vector<infrast::Smiley> result;
    for (const auto& [rect, _, templ_name] : analyzer.get_result()) {
        result.emplace_back(infrast::Smiley { templ_name_to_smiley_type.at(templ_name), rect });
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(rect), cv::Scalar(0, 0, 255), 2);
#endif
    }

    m_result = std::move(result);
    return true;
}
