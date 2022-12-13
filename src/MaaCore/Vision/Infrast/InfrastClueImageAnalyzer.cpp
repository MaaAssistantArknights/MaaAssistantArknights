#include "InfrastClueImageAnalyzer.h"

#include "Vision/MultiMatchImageAnalyzer.h"

bool asst::InfrastClueImageAnalyzer::analyze()
{
    clue_detect();
    if (m_need_detailed) {
        clue_analyze();
    }

    return !m_result.empty();
}

bool asst::InfrastClueImageAnalyzer::clue_detect()
{
    MultiMatchImageAnalyzer clue_analyzer(m_image);
    clue_analyzer.set_task_info("InfrastClue");

    clue_analyzer.set_roi(m_roi); // 该任务以外部设置的roi为准
    if (!clue_analyzer.analyze()) {
        return false;
    }
    clue_analyzer.sort_result_horizontal();
    for (const auto& res : clue_analyzer.get_result()) {
        m_result.emplace_back(res.rect, std::string());
    }

    return !m_result.empty();
}

bool asst::InfrastClueImageAnalyzer::clue_analyze()
{
    // todo
    return false;
}
