#include "RecruitImageAnalyzer.h"

#include "MatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "Resource.h"

bool asst::RecruitImageAnalyzer::analyze()
{
    m_tags_result.clear();
    m_set_time_rect.clear();

    time_analyze();
    refresh_analyze();
    bool ret = tags_analyze();
    ret &= confirm_analyze();

    return ret;
}

bool asst::RecruitImageAnalyzer::tags_analyze()
{
    static bool analyzer_inited = false;
    static OcrImageAnalyzer tags_analyzer;
    if (!analyzer_inited) {
        const auto tags_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(
            Task.get("RecruitTags"));
        tags_analyzer.set_roi(tags_task_ptr->roi);
        auto& all_tags_set = Resrc.recruit().get_all_tags();
        std::vector<std::string> all_tags_vec;
        all_tags_vec.assign(all_tags_set.begin(), all_tags_set.end());
        // 把因为“资深干员”是“高级资深干员”的子串，把“高级资深干员”放到最前面，免得先被“资深干员”匹配上了
        auto ssr_iter = std::find(all_tags_vec.begin(), all_tags_vec.end(), "高级资深干员");
        if (ssr_iter != all_tags_vec.end()) {
            std::swap(*ssr_iter, all_tags_vec.front());
        }

        tags_analyzer.set_required(std::move(all_tags_vec));
        tags_analyzer.set_replace(tags_task_ptr->replace_map);
        analyzer_inited = true;
    }

    tags_analyzer.set_image(m_image);

    if (tags_analyzer.analyze()) {
        m_tags_result = tags_analyzer.get_result();
        return true;
        //if (m_tags_result.size() == Resrc.recruit().CorrectNumberOfTags) {
        //    return true;
        //}
    }
    return false;
}

bool asst::RecruitImageAnalyzer::time_analyze()
{
    const auto time_task_ptr = Task.get("RecruitTime");

    MatchImageAnalyzer time_analyzer(m_image);
    time_analyzer.set_task_info(time_task_ptr);

    if (time_analyzer.analyze()) {
        Rect rect = time_analyzer.get_result().rect;
        const auto& res_move = time_task_ptr->rect_move;
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

bool asst::RecruitImageAnalyzer::confirm_analyze()
{
    MatchImageAnalyzer confirm_analyzer(m_image);
    confirm_analyzer.set_task_info("RecruitConfirm");

    if (confirm_analyzer.analyze()) {
        m_confirm_rect = confirm_analyzer.get_result().rect;
        return true;
    }

    return false;
}

bool asst::RecruitImageAnalyzer::refresh_analyze()
{
    MatchImageAnalyzer refresh_analyzer(m_image);

    refresh_analyzer.set_task_info("RecruitRefresh");

    if (refresh_analyzer.analyze()) {
        m_refresh_rect = refresh_analyzer.get_result().rect;
        return true;
    }

    return false;
}
