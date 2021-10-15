#include "InfrastSkillsImageAnalyzer.h"

#include "Resource.h"
#include "InfrastSmileyImageAnalyzer.h"

bool asst::InfrastSkillsImageAnalyzer::analyze()
{
    const auto upper_task_ptr = resource.task().task_ptr("InfrastSkillsUpper");
    const auto lower_task_ptr = resource.task().task_ptr("InfrastSkillsLower");

    bool upper_ret = skills_roi_analyze(upper_task_ptr->roi);
    bool lower_ret = skills_roi_analyze(lower_task_ptr->roi);

    return upper_ret && lower_ret;
}

bool asst::InfrastSkillsImageAnalyzer::skills_roi_analyze(const Rect& roi)
{
    InfrastSmileyImageAnalyzer smiley_analyzer(m_image, roi);

    if (!smiley_analyzer.analyze()) {
        return false;
    }

    const auto& smiley_res = smiley_analyzer.get_result();

    return true;
}