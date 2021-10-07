#include "RecruitImageAnalyzer.h"

#include "Resource.h"
#include "OcrImageAnalyzer.h"
#include "MatchImageAnalyzer.h"

bool asst::RecruitImageAnalyzer::analyze()
{
    bool time_ret = time_analyze();
    bool tags_ret = tags_analyze();

    return time_ret || tags_ret;
}

bool asst::RecruitImageAnalyzer::tags_analyze()
{
    const static auto tags_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(
        resource.task().task_ptr("RecruitTags"));

    static bool analyzer_inited = false;
    static OcrImageAnalyzer tags_analyzer;
    if (!analyzer_inited) {
        tags_analyzer.set_roi(tags_task_ptr->roi);
        auto& all_tags_set = resource.recruit().get_all_tags();
        std::vector<std::string> all_tags_vec;
        all_tags_vec.assign(all_tags_set.begin(), all_tags_set.end());
        tags_analyzer.set_required(std::move(all_tags_vec));
        tags_analyzer.set_replace(tags_task_ptr->replace_map);
        analyzer_inited = true;
    }

    tags_analyzer.set_image(m_image);

    if (tags_analyzer.analyze()) {
        m_tags_result = tags_analyzer.get_result();
        return true;
        //if (m_tags_result.size() == resource.recruit().CorrectNumberOfTags) {
        //    return true;
        //}
    }
    return false;
}

bool asst::RecruitImageAnalyzer::time_analyze()
{
    const static auto time_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("RecruitTime"));

    MatchImageAnalyzer time_analyzer(
        m_image,
        time_task_ptr->roi,
        time_task_ptr->templ_name,
        time_task_ptr->templ_threshold);

    if (time_analyzer.analyze()) {
        Rect rect = time_analyzer.get_result().rect;
        const auto& res_move = time_task_ptr->result_move;
        if (!res_move.empty()) {
            rect.x += res_move.x;
            rect.y += res_move.y;
            rect.width = res_move.width;
            rect.height = res_move.height;
        }
        m_set_time_rect.emplace_back(rect);
        return true;
    }
    return false;
}