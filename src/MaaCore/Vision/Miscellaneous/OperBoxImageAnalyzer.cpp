#include "OperBoxImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/MatchImageAnalyzer.h"
#include "Vision/OcrWithFlagTemplImageAnalyzer.h"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

bool asst::OperBoxImageAnalyzer::analyze()
{
    LogTraceFunction;
    m_current_page_opers.clear();
    bool oper_box = analyzer_opers();
#ifdef ASST_DEBUG
    m_image_draw = m_image_draw_oper;
#endif
    save_img(utils::path("debug") / utils::path("oper"));
    return oper_box;
}

bool asst::OperBoxImageAnalyzer::analyzer_opers()
{
    std::vector<asst::TextRect> oper_names_result;
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");

    OcrWithFlagTemplImageAnalyzer oper_name_analyzer(m_image);

    oper_name_analyzer.set_task_info("OperBoxFlagLV", "OperBoxNameOCR");
    oper_name_analyzer.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
    const auto& all_opers = BattleData.get_all_oper_names();
    oper_name_analyzer.set_required(std::vector(all_opers.begin(), all_opers.end()));
    if (!oper_name_analyzer.analyze()) {
        return false;
    }

    for (auto& opername : oper_name_analyzer.get_result()) {
        OperBoxInfo oper_info;
        oper_info.name = std::move(opername.text);
        m_current_page_opers.emplace_back(oper_info);

#ifdef ASST_DEBUG
        m_image_draw_oper = m_image;
        cv::rectangle(m_image_draw_oper, make_rect<cv::Rect>(opername.rect), cv::Scalar(0, 255, 0), 2);
        cv::putText(m_image_draw_oper, opername.text, cv::Point(opername.rect.x, opername.rect.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
#endif
    }
    return true;
}
