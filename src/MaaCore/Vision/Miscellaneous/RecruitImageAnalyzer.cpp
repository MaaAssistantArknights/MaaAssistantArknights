#include "RecruitImageAnalyzer.h"

#include "Config/Miscellaneous/RecruitConfig.h"
#include "Config/TaskData.h"
#include "Vision/MatchImageAnalyzer.h"
#include "Vision/MultiMatchImageAnalyzer.h"
#include "Vision/OcrImageAnalyzer.h"

bool asst::RecruitImageAnalyzer::analyze()
{
    m_tags_result.clear();

    time_analyze();
    refresh_analyze();
    bool ret = tags_analyze();

    return ret;
}

bool asst::RecruitImageAnalyzer::tags_analyze()
{
    static bool analyzer_inited = false;
    static OcrImageAnalyzer tags_analyzer;
    if (!analyzer_inited) {
        tags_analyzer.set_task_info("RecruitTags");
        auto& all_tags_set = RecruitData.get_all_tags();

        // 已经 fullMatch，不会再把 `支援机械` 匹配成 `支援`、`高级资深干员` 匹配成 `资深干员` 了，因此不必再排序。
        tags_analyzer.set_required(std::vector(all_tags_set.begin(), all_tags_set.end()));
        analyzer_inited = true;
    }

    tags_analyzer.set_image(m_image);

    if (tags_analyzer.analyze()) {
        m_tags_result = tags_analyzer.get_result();
        return true;
        // if (m_tags_result.size() == RecruitData.CorrectNumberOfTags) {
        //     return true;
        // }
    }
    return false;
}

bool asst::RecruitImageAnalyzer::time_analyze()
{
    MultiMatchImageAnalyzer decrement_a(m_image);
    decrement_a.set_task_info("RecruitTimerDecrement");
    if (!decrement_a.analyze()) return false;
    if (decrement_a.get_result().size() != 2) return false; // expecting two buttons
    decrement_a.sort_result_horizontal();
    m_hour_decrement = decrement_a.get_result().at(0).rect;
    m_minute_decrement = decrement_a.get_result().at(1).rect;
    return true;
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
