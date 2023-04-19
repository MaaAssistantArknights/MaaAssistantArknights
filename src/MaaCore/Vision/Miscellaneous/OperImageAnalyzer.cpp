#include "OperImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Vision/MatchImageAnalyzer.h"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

#include <Vision/OcrWithFlagTemplImageAnalyzer.h>
#include <numbers>

bool asst::OperImageAnalyzer::analyze()
{
    LogTraceFunction;
    current_page_opers.clear();
    bool oper = analyzer_opers();
#ifdef ASST_DEBUG
    m_image_draw = m_image_draw_oper;
#endif
    save_img(utils::path("debug") / utils::path("oper"));
    return oper;
}

bool asst::OperImageAnalyzer::analyzer_opers()
{
    std::vector<asst::TextRect> oper_names_result;
    oper_names_result.clear();
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");

    OcrWithFlagTemplImageAnalyzer oper_name_analyzer(m_image);

    oper_name_analyzer.set_task_info("OperatorNameFlagLV", "OperatorOCRLV");
    oper_name_analyzer.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
    oper_name_analyzer.analyze();

    oper_names_result = oper_name_analyzer.get_result();
    // 按位置排个序-先直接copy代码过来，后面可能重构 （
    // +---+
    // |1 3|
    // |2 4|
    // +---+
    ranges::sort(oper_names_result, [](const TextRect& lhs, const TextRect& rhs) -> bool {
        if (std::abs(lhs.rect.x - rhs.rect.x) < 5) { // x差距较小则理解为是同一排的，按y排序
            return lhs.rect.y < rhs.rect.y;
        }
        else {
            return lhs.rect.x < rhs.rect.x;
        }
    });

    if (oper_names_result.empty()) {
        return false;
    }

    for (auto& opername : oper_names_result) {
        OperBoxInfo oper_info;
        oper_info.name = std::move(opername.text);
        current_page_opers.emplace_back(oper_info);
#ifdef ASST_DEBUG
        m_image_draw_oper = m_image;
        cv::rectangle(m_image_draw_oper, make_rect<cv::Rect>(opername.rect), cv::Scalar(0, 255, 0), 2);
        cv::putText(m_image_draw_oper, opername.text, cv::Point(opername.rect.x, opername.rect.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
#endif
    }
    return true;
}
