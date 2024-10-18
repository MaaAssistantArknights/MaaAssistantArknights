#include "SupportListAnalyzer.h"

#include "Config/TaskData.h"
#include "Utils/NoWarningCV.h"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

bool asst::SupportListAnalyzer::analyze()
{
    LogTraceFunction;

    MultiMatcher flag_analyzer(m_image);
    flag_analyzer.set_task_info("SupportListAnalyzer-Flag");
    if (!flag_analyzer.analyze()) {
        return false;
    }
    MultiMatcher::ResultsVec flags = flag_analyzer.get_result();

    const auto name_task_ptr = Task.get<OcrTaskInfo>("SupportListAnalyzer-NameOcr");
    const auto elite_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-Elite");
    const auto level_task_ptr = Task.get<OcrTaskInfo>("SupportListAnalyzer-LevelOcr");

    RegionOCRer ocr_analyzer(m_image);
    Matcher elite_analyzer(m_image);

    std::vector<SupportUnit> results;
    for (const auto& [rect, score, templ_name] : flags) {
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(rect), cv::Scalar(0, 255, 0), 2);
#endif

        // ————————————————————————————————————————————————————————————————
        // 干员名称
        // ————————————————————————————————————————————————————————————————
        const Rect name_roi = { rect.x + name_task_ptr->roi.x,
                                rect.y - name_task_ptr->roi.y,
                                name_task_ptr->roi.width,
                                name_task_ptr->roi.height };
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(name_roi), cv::Scalar(0, 255, 0), 2);
#endif

        ocr_analyzer.set_roi(name_roi);
        ocr_analyzer.set_replace(name_task_ptr->replace_map, name_task_ptr->replace_full);
        if (!ocr_analyzer.analyze()) {
            continue;
        }
        const std::string name = ocr_analyzer.get_result().text;

        // ————————————————————————————————————————————————————————————————
        // 干员精英化等级
        // ————————————————————————————————————————————————————————————————
        const Rect elite_roi = { rect.x + elite_task_ptr->roi.x,
                                rect.y - elite_task_ptr->roi.y,
                                elite_task_ptr->roi.width,
                                elite_task_ptr->roi.height };
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(elite_roi), cv::Scalar(0, 255, 0), 2);
#endif


        elite_analyzer.set_task_info(elite_task_ptr);
        elite_analyzer.set_roi(elite_roi);
        int elite = 0;
        if (elite_analyzer.analyze()) {
            elite = (elite_analyzer.get_result().templ_name == "SupportListAnalyzer-Elite-2" ? 2 : 1);
        }

        // ————————————————————————————————————————————————————————————————
        // 干员等级
        // ————————————————————————————————————————————————————————————————
        const Rect level_roi = { rect.x + level_task_ptr->roi.x,
                                 rect.y - level_task_ptr->roi.y,
                                 level_task_ptr->roi.width,
                                 level_task_ptr->roi.height };
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(level_roi), cv::Scalar(0, 255, 0), 2);
#endif

        ocr_analyzer.set_roi(level_roi);
        ocr_analyzer.set_replace(level_task_ptr->replace_map, level_task_ptr->replace_full);
        if (!ocr_analyzer.analyze()) {
            continue;
        }
        int level = 0;
        if (!utils::chars_to_number(ocr_analyzer.get_result().text, level)) {
            continue;
        }
        Log.info(ocr_analyzer.get_result().text, level);

        SupportUnit support_unit;
        support_unit.rect = rect;
        support_unit.name = name;
        support_unit.elite = elite;
        support_unit.level = level;
        support_unit.from_friend = (templ_name == "SupportListAnalyzer-Friend.png");
        Log.info(
            __FUNCTION__,
            "| Found support operator from",
            support_unit.from_friend ? "friend:" : "non-friend:",
            support_unit.name,
            "with elite",
            support_unit.elite,
            "and level",
            support_unit.level);
        results.emplace_back(std::move(support_unit));
    }

    m_result = std::move(results);
    return !m_result.empty();
}
