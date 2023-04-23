#include "OperBoxImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/BestMatchImageAnalyzer.h"
#include "Vision/MatchImageAnalyzer.h"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

asst::OperBoxImageAnalyzer::OperBoxImageAnalyzer(const cv::Mat& image)
    : OcrWithPreprocessImageAnalyzer(image), m_multi_match_image_analyzer(image)
{}
bool asst::OperBoxImageAnalyzer::analyze()
{
    LogTraceFunction;
    m_current_page_opers.clear();
    m_lv_flags.clear();

    bool oper_box = analyzer_oper_box();

#ifdef ASST_DEBUG
    m_image_draw = m_image_draw_oper;
#endif
    save_img(utils::path("debug") / utils::path("oper"));
    return oper_box;
}

int asst::OperBoxImageAnalyzer::level_num(std::string level)
{
    if (level.empty() || !ranges::all_of(level, [](const char& c) -> bool { return std::isdigit(c) })) {
        return 1;
    }
    return std::stoi(level);
}

bool asst::OperBoxImageAnalyzer::analyzer_oper_box()
{
    LogTraceFunction;
#ifdef ASST_DEBUG
    m_image_draw_oper = m_image_draw;
#endif // ASST_DEBUG

    // lv_flag
    m_multi_match_image_analyzer.set_task_info("OperBoxFlagLV");
    m_multi_match_image_analyzer.analyze();
    m_lv_flags = m_multi_match_image_analyzer.get_result();

    if (m_lv_flags.empty()) {
        return false;
    }

    // 填入rect
    for (const auto& lv_flag : m_lv_flags) {
        OperBoxInfo box;
        box.rect = lv_flag.rect;
        box.own = true;
        m_current_page_opers.emplace_back(box);

#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw_oper, make_rect<cv::Rect>(lv_flag.rect), cv::Scalar(0, 255, 0), 2);
#endif // ASST_DEBUG
    }

    // 识别名字
    auto ocr_task_ptr_name = Task.get<OcrTaskInfo>("OperBoxNameOCR");
    set_task_info(*ocr_task_ptr_name);
    Rect name_roi = ocr_task_ptr_name->roi;

    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");
    OcrWithPreprocessImageAnalyzer::set_image(m_image);
    OcrWithPreprocessImageAnalyzer::set_replace(ocr_replace->replace_map, ocr_replace->replace_full);

    for (auto& box : m_current_page_opers) {
        Rect roi = box.rect.move(name_roi);
        if (roi.x + roi.width >= WindowWidthDefault) {
            continue;
        }
        OcrWithPreprocessImageAnalyzer::set_roi(roi);
        if (OcrWithPreprocessImageAnalyzer::analyze()) {
            const auto& ocr_result = m_ocr_result.begin();
            std::string name = ocr_result->text;
            box.id = BattleData.get_id(name);
            box.name = std::move(name);
#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw_oper, make_rect<cv::Rect>(ocr_result->rect), cv::Scalar(0, 255, 0), 2);
            cv::putText(m_image_draw_oper, box.id, cv::Point(roi.x, roi.y + 35), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                        cv::Scalar(0, 0, 255), 2);
#endif // ASST_DEBUG
        }
    }

    // 识别等级
    const auto ocr_task_ptr_level = Task.get<OcrTaskInfo>("OperBoxLevelOCR");
    set_task_info(*ocr_task_ptr_level);
    Rect level_roi = ocr_task_ptr_level->roi;

    const auto& ocr_replace_num = Task.get<OcrTaskInfo>("NumberOcrReplace");
    OcrWithPreprocessImageAnalyzer::set_use_char_model(true);
    OcrWithPreprocessImageAnalyzer::set_replace(ocr_replace_num->replace_map, ocr_replace_num->replace_full);

    for (auto& box : m_current_page_opers) {
        Rect roi = box.rect.move(level_roi);
        if (roi.x + roi.width >= WindowWidthDefault) {
            continue;
        }
        OcrWithPreprocessImageAnalyzer::set_roi(roi);
        if (OcrWithPreprocessImageAnalyzer::analyze()) {
            const auto& ocr_result = m_ocr_result.begin();
            std::string level = ocr_result->text;
            box.level = std::move(level_num(level));
#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw_oper, make_rect<cv::Rect>(ocr_result->rect), cv::Scalar(0, 255, 0), 1);
            cv::putText(m_image_draw_oper, level, cv::Point(roi.x, roi.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                        cv::Scalar(0, 0, 255), 2);
#endif // ASST_DEBUG
        }
    }

    // 识别精英度
    const std::string task_name = "OperBoxFlagElite";
    Rect elite_roi = Task.get(task_name)->roi;
    BestMatchImageAnalyzer elite_analyzer(m_image);
    elite_analyzer.set_task_info(task_name);
    elite_analyzer.append_templ("OperBoxFlagElite1.png");
    elite_analyzer.append_templ("OperBoxFlagElite2.png");
    for (auto& box : m_current_page_opers) {
        Rect roi = box.rect.move(elite_roi);
        elite_analyzer.set_roi(roi);
        if (!elite_analyzer.analyze()) {
            box.elite = 0;
        }
        else {
            const auto& elite_templ = elite_analyzer.get_result();
            std::string elite = elite_templ.name.substr(task_name.size(), 1);
            box.elite = std::stoi(elite);
#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw_oper, make_rect<cv::Rect>(elite_templ.templ), cv::Scalar(0, 255, 0), 1);
#endif // ASST_DEBUG  
        }
#ifdef ASST_DEBUG
        cv::putText(m_image_draw_oper, std::to_string(box.elite), cv::Point(roi.x, roi.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
#endif // ASST_DEBUG
    }

    // 识别潜能
    const std::string task_name_p = "OperBoxPotential";
    Rect potential_roi = Task.get(task_name_p)->roi;
    BestMatchImageAnalyzer potential_analyzer(m_image);
    potential_analyzer.set_task_info(task_name_p);
    for (int i = 1; i < 6; i++) {
        std::string potential_temp_name = task_name_p + std::to_string(i) + ".png";
        potential_analyzer.append_templ(potential_temp_name);
    }
    for (auto& box : m_current_page_opers) {
        Rect roi = box.rect.move(potential_roi);
        potential_analyzer.set_roi(roi);
        if (!potential_analyzer.analyze()) {
            box.potential = 0;
        }
        else {
            const auto& poten_templ = potential_analyzer.get_result();
            std::string potential = poten_templ.name.substr(task_name_p.size(), 1);
            box.potential = std::stoi(potential);
#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw_oper, make_rect<cv::Rect>(poten_templ.templ), cv::Scalar(0, 255, 0), 1);
#endif // ASST_DEBUG
        }
#ifdef ASST_DEBUG
        cv::putText(m_image_draw_oper, std::to_string(box.potential), cv::Point(roi.x, roi.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
#endif // ASST_DEBUG
    }

    sort_oper_horizontal(m_current_page_opers);
    for (const auto& box : m_current_page_opers) {
        Log.trace("operBox{", "\"rect_lv\":[", box.rect.x, box.rect.y, box.rect.width, box.rect.height,
                  "], \"id\": ", box.id, ", \"name\": ", box.name, ", \"elite\": ", box.elite,
                  ", \"level\": ", box.level, ", \"potential\": ", box.potential, "}");
    }
    return !m_current_page_opers.empty();
}

void asst::OperBoxImageAnalyzer::sort_oper_horizontal(std::vector<asst::OperBoxInfo> m_oper_boxs)
{
    // 按位置排个序
    ranges::sort(m_oper_boxs, [](const OperBoxInfo& lhs, const OperBoxInfo& rhs) -> bool {
        if (std::abs(lhs.rect.y - rhs.rect.y) < 5) { // y差距较小则理解为是同一排的，按x排序
            return lhs.rect.x < rhs.rect.x;
        }
        else {
            return lhs.rect.y < rhs.rect.y;
        }
    });
}
