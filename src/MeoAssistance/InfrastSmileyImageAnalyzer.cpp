#include "InfrastSmileyImageAnalyzer.h"

#include "Resource.h"
#include "MultiMatchImageAnalyzer.h"
#include "AsstUtils.hpp"

bool asst::InfrastSmileyImageAnalyzer::analyze()
{
    const static std::unordered_map<InfrastSmileyType, std::string> smiley_map =
    {
        {InfrastSmileyType::Rest, "InfrastSmileyOnRest"},
        {InfrastSmileyType::Work, "InfrastSmileyOnWork"},
        {InfrastSmileyType::Distract, "InfrastSmileyOnDistract"}
    };

    m_result.clear();

    MultiMatchImageAnalyzer mm_analyzer(m_image, m_roi);

    decltype(m_result) temp_result;
    for (const auto& [type, task_name] : smiley_map) {
        const auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(resource.task().task_ptr(task_name));
        mm_analyzer.set_templ_name(task_ptr->templ_name);
        mm_analyzer.set_threshold(task_ptr->templ_threshold);
        mm_analyzer.set_mask_range(task_ptr->mask_range);
        if (!mm_analyzer.analyze()) {
            continue;
        }
        auto& res = mm_analyzer.get_result();
        for (const MatchRect& mr : res) {
            temp_result.emplace_back(InfrastSmileyInfo{ type, mr.rect });
#ifdef LOG_TRACE
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(mr.rect), cv::Scalar(0, 0, 255), 2);
#endif
        }
    }
    m_result = std::move(temp_result);
    return true;
}