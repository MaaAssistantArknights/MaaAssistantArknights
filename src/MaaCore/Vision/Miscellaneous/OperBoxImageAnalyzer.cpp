#include "OperBoxImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Vision/MatchImageAnalyzer.h"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

#include <Vision/OcrWithFlagTemplImageAnalyzer.h>

#include <numbers>

asst::OperBoxImageAnalyzer::OperBoxImageAnalyzer(const cv::Mat& image)
    : OcrWithPreprocessImageAnalyzer(image), m_multi_match_image_analyzer(image)
{}
bool asst::OperBoxImageAnalyzer::analyze()
{
    LogTraceFunction;
    oper_box_clear();
    //bool oper_box = analyzer_opers();
    //bool oper_box = analyzer_opers_box();
    bool oper_box = analyzer_box();
#ifdef ASST_DEBUG
    draft_img();
    m_image_draw = m_image_draw_oper;
#endif
    save_img(utils::path("debug") / utils::path("oper"));
    return oper_box;
}

bool asst::OperBoxImageAnalyzer::analyzer_opers()
{
    std::vector<asst::TextRect> oper_names_result;
    oper_names_result.clear();
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");

    OcrWithFlagTemplImageAnalyzer oper_name_analyzer(m_image);

    oper_name_analyzer.set_task_info("OperBoxFlagLV", "OperBoxNameOCR");
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
bool asst::OperBoxImageAnalyzer::analyzer_opers_box()
{
    std::vector<asst::TextRect> oper_names_result;
    std::vector<asst::TextRect> oper_levels_result;
    oper_names_result.clear();
    oper_levels_result.clear();
    //获取lv标志位置
    m_multi_match_image_analyzer.set_image(m_image);
    m_multi_match_image_analyzer.set_task_info("OperBoxFlagLV");
    m_multi_match_image_analyzer.analyze();
    auto& oper_box_flag_lvs = m_multi_match_image_analyzer.get_result();

    
    if (oper_box_flag_lvs.empty()) {
        return false;
    }
    //识别名字
    auto ocr_task_ptr_name = Task.get<OcrTaskInfo>("OperBoxNameOCR");
    set_task_info(*ocr_task_ptr_name);
    Rect name_roi = ocr_task_ptr_name->roi;

    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");
    OcrWithPreprocessImageAnalyzer::set_image(m_image);
    OcrWithPreprocessImageAnalyzer::set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
    
    
    for (const auto& box : oper_box_flag_lvs) {
        Rect roi = box.rect.move(name_roi);
        if (roi.x + roi.width >= WindowWidthDefault) {
            continue;
        }
        OcrWithPreprocessImageAnalyzer::set_roi(roi);
        if (OcrWithPreprocessImageAnalyzer::analyze()) {
            oper_names_result.insert(oper_names_result.end(), std::make_move_iterator(m_ocr_result.begin()),
                                     std::make_move_iterator(m_ocr_result.end()));
        }

    }

    // 将lv和名字按顺序排列
    sort_result_horizontal_m(oper_box_flag_lvs); // 取rect
    sort_result_horizontal_t(oper_names_result); // 取name和id

    for (auto& opername : oper_names_result) {
        /*OperBoxInfo oper_info;
        std::string name = opername.text;
        std::string id = BattleData.get_id(name);
        oper_info.name = std::move(name);
        oper_info.id = id;
        m_current_page_opers.emplace_back(oper_info);*/
        //m_result.emplace(std::move(id), std::move(oper_info));
#ifdef ASST_DEBUG
        m_image_draw_oper = m_image;
        cv::rectangle(m_image_draw_oper, make_rect<cv::Rect>(opername.rect), cv::Scalar(0, 255, 0), 2);
        cv::putText(m_image_draw_oper, opername.text, cv::Point(opername.rect.x, opername.rect.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
#endif
    }

    


    //识别等级
    auto ocr_task_ptr_level = Task.get<OcrTaskInfo>("OperBoxLevelOCR");
    set_task_info(*ocr_task_ptr_level);
    Rect level_roi = ocr_task_ptr_level->roi;

    const auto& ocr_replace_num = Task.get<OcrTaskInfo>("NumberOcrReplace");
    OcrWithPreprocessImageAnalyzer::set_replace(ocr_replace_num->replace_map, ocr_replace_num->replace_full);


    for (const auto& box : oper_box_flag_lvs) {
        Rect roi = box.rect.move(level_roi);
        if (roi.x + roi.width >= WindowWidthDefault) {
            continue;
        }
        OcrWithPreprocessImageAnalyzer::set_roi(roi);
        if (OcrWithPreprocessImageAnalyzer::analyze()) {
            oper_levels_result.insert(oper_levels_result.end(), std::make_move_iterator(m_ocr_result.begin()),
                                     std::make_move_iterator(m_ocr_result.end()));
        }
    }
    for (auto& operlevel : oper_levels_result) {
        /*OperBoxInfo oper_info;
        oper_info.name = std::move(operlevel.text);
        m_current_page_opers.emplace_back(oper_info);*/
#ifdef ASST_DEBUG
        //m_image_draw_oper = m_image;
        cv::rectangle(m_image_draw_oper, make_rect<cv::Rect>(operlevel.rect), cv::Scalar(0, 255, 0), 2);
        cv::putText(m_image_draw_oper, operlevel.text, cv::Point(operlevel.rect.x, operlevel.rect.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
#endif
    }
    //将等级排序
    sort_result_horizontal_t(oper_levels_result);

    //三者长度不等，则识别有误，退出
    if (oper_box_flag_lvs.size() != oper_names_result.size() || oper_names_result.size() != oper_levels_result.size()) {
        return false;
    }
    /*auto& flag_itr = oper_box_flag_lvs.begin();
    auto& lv_itr = oper_levels_result.begin();
    auto& name_itr = oper_names_result.begin();*/
    size_t length = oper_box_flag_lvs.size();
    int i = 0;
    while (i<length) {
        OperBoxInfo oper_info;
        std::string name = oper_names_result[i].text;
        std::string id = BattleData.get_id(name);
        oper_info.name = std::move(name);
        oper_info.id = id;
        oper_info.rect = oper_box_flag_lvs[i].rect;
        oper_info.own = true;
        oper_info.level = level_num(oper_levels_result[i].text);

        m_current_page_opers.emplace_back(oper_info);
        //先不在这里插入，无序不好处理
        /*m_result.emplace(std::move(id), std::move(oper_info));*/

        i++;
    }
    //顺序不能乱
    
    // 获取精英一
    m_multi_match_image_analyzer.set_task_info("OperBoxFlagElitismOne");
    m_multi_match_image_analyzer.analyze();
    // auto& oper_box_flag_lvs = m_multi_match_image_analyzer.get_result();
    auto& elite_one_result = m_multi_match_image_analyzer.get_result();

    // 获取精英二
    m_multi_match_image_analyzer.set_task_info("OperBoxFlagElitismTwo");
    m_multi_match_image_analyzer.analyze();
    m_multi_match_image_analyzer.save_img();
    auto& elite_two_result = m_multi_match_image_analyzer.get_result();

    
    //排序，与lvflag一致
    // 1 2 3
    // 4 5 6
    // 精英一空，精英二空
    int elite_one_count = 0;
    int elite_two_count = 0;
    if (!elite_one_result.empty()) {
        sort_result_horizontal_m(elite_one_result);
    }
    if (!elite_two_result.empty()) {
        sort_result_horizontal_m(elite_two_result);
    }
    
    //循环将精英度加入box
    for (auto&  box_info : m_current_page_opers) {
        //判断是否有值
        Rect lv_rect = box_info.rect;
        if (elite_one_count < elite_one_result.size()) {
            //判断该rect是否与第一个lv接近，接近则赋值，+1
            Rect elite_rect = elite_one_result[elite_one_count].rect;
            if (std::abs(lv_rect.x - elite_rect.x) < 10 && std::abs(lv_rect.y - elite_rect.y) < 55) {
                //符合第一个就不用判断第二个了
                elite_one_count++;
                box_info.elite = 1;
                continue;
            }
        }
        if (elite_one_count < elite_two_result.size()) {
            // 判断该rect是否与第一个lv接近，接近则赋值，+1
            Rect elite_rect = elite_two_result[elite_two_count].rect;
            if (std::abs(lv_rect.x - elite_rect.x) < 10 && std::abs(lv_rect.y - elite_rect.y) < 55) {
                // 符合第一个就不用判断第二个了
                elite_two_count++;
                box_info.elite = 2;
            }
        }
        Log.info("operBox: name:", box_info.name, ", elite: ", box_info.elite, ", level: ", box_info.level,
                 ",id: ", box_info.id);
        m_result.emplace(box_info.id, std::move(box_info));

    }

    return true;
}

bool asst::OperBoxImageAnalyzer::analyzer_box()
{
    //获取所有rect
    if (!get_all_info()) {
        return false;
    }
    //排序
    sort_all();
    //组装干员box
    size_t count = m_lv_flags.size();
    //数量不匹配退出
    if (count != m_oper_levels.size() || count !=m_oper_names.size()) {
        return false;
    }

    //精一，精二,循环计数
    //int elite_one_flag = 0, elite_two_flag = 0;
    int k=0;

    while (k < count) {
        OperBoxInfo oper_info;
        std::string name = m_oper_names[k].text;
        std::string id = BattleData.get_id(name);
        Rect lv_rect = m_lv_flags[k].rect;
        oper_info.name = std::move(name);
        oper_info.id = id;
        oper_info.rect = lv_rect;
        oper_info.own = true;
        oper_info.level = level_num(m_oper_levels[k].text);

        //插入精英度1
        /*if (elite_one_flag < m_elite_ones.size()) {
            if (is_near(lv_rect, m_elite_ones[elite_one_flag].rect)) {
                elite_one_flag++;
                oper_info.elite = 1;
            }
        }
        if (elite_two_flag < m_elite_twos.size()) {
            if (is_near(lv_rect, m_elite_twos[elite_two_flag].rect)) {
                elite_two_flag++;
                oper_info.elite = 2;
            }
        }*/
        if (!m_elite_ones.empty()) {
            for (const auto& elite_one : m_elite_ones) {
                if (is_near(elite_one.rect,lv_rect)) {
                    oper_info.elite = 1;
                }
            }
        }
        if (!m_elite_twos.empty()) {
            for (const auto& elite_two : m_elite_twos) {
                if (is_near(elite_two.rect, lv_rect)) {
                    oper_info.elite = 2;
                }
            }
        }
        Log.info("operBox: name:", oper_info.name, ", elite: ", oper_info.elite, ", level: ", oper_info.level,
                 ",id: ", oper_info.id);
        m_current_page_opers.emplace_back(oper_info);
        k++;
    }
    
    return !m_current_page_opers.empty();
}

void asst::OperBoxImageAnalyzer::sort_result_horizontal_m(std::vector<asst::MatchRect> m_rect_box)
{
    // 按位置排个序
    ranges::sort(m_rect_box, [](const MatchRect& lhs, const MatchRect& rhs) -> bool {
        if (std::abs(lhs.rect.y - rhs.rect.y) < 5) { // y差距较小则理解为是同一排的，按x排序
            return lhs.rect.x < rhs.rect.x;
        }
        else {
            return lhs.rect.y < rhs.rect.y;
        }
    });
}

void asst::OperBoxImageAnalyzer::sort_result_horizontal_t(std::vector<asst::TextRect> t_rect_box)
{
    // 按位置排个序
    ranges::sort(t_rect_box, [](const TextRect& lhs, const TextRect& rhs) -> bool {
        if (std::abs(lhs.rect.y - rhs.rect.y) < 5) { // y差距较小则理解为是同一排的，按x排序
            return lhs.rect.x < rhs.rect.x;
        }
        else {
            return lhs.rect.y < rhs.rect.y;
        }
    });
}

//转换为整数。
int asst::OperBoxImageAnalyzer::level_num(std::string level)
{
    try {
        return std::stoi(level);
    }
    catch(std::invalid_argument e) {
        //解析不出来，则默认为1
        return 1;
    }
}
void asst::OperBoxImageAnalyzer::oper_box_clear() {
    m_oper_names.clear();
    m_oper_levels.clear();
    m_lv_flags.clear();
    m_elite_ones.clear();
    m_elite_twos.clear();
}

void asst::OperBoxImageAnalyzer::sort_all() {
    sort_result_horizontal_m(m_lv_flags);
    sort_result_horizontal_t(m_oper_names);
    sort_result_horizontal_t(m_oper_levels);
    if (!m_elite_ones.empty()) {
        sort_result_horizontal_m(m_elite_ones);
    }
    if (!m_elite_twos.empty()) {
        sort_result_horizontal_m(m_elite_twos);
    }
}

bool asst::OperBoxImageAnalyzer::get_all_info()
{
    // lv_flag
    m_multi_match_image_analyzer.set_task_info("OperBoxFlagLV");
    m_multi_match_image_analyzer.analyze();
    m_lv_flags = m_multi_match_image_analyzer.get_result();

    if (m_lv_flags.empty()) {
        return false;
    }

    // 识别名字
    auto ocr_task_ptr_name = Task.get<OcrTaskInfo>("OperBoxNameOCR");
    set_task_info(*ocr_task_ptr_name);
    Rect name_roi = ocr_task_ptr_name->roi;

    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");
    OcrWithPreprocessImageAnalyzer::set_image(m_image);
    OcrWithPreprocessImageAnalyzer::set_replace(ocr_replace->replace_map, ocr_replace->replace_full);

    for (const auto& box : m_lv_flags) {
        Rect roi = box.rect.move(name_roi);
        if (roi.x + roi.width >= WindowWidthDefault) {
            continue;
        }
        OcrWithPreprocessImageAnalyzer::set_roi(roi);
        if (OcrWithPreprocessImageAnalyzer::analyze()) {
            m_oper_names.insert(m_oper_names.end(), std::make_move_iterator(m_ocr_result.begin()),
                                std::make_move_iterator(m_ocr_result.end()));
        }
    }
    if (m_oper_names.empty()) {
        return false;
    }

    // 识别等级
    auto ocr_task_ptr_level = Task.get<OcrTaskInfo>("OperBoxLevelOCR");
    set_task_info(*ocr_task_ptr_level);
    Rect level_roi = ocr_task_ptr_level->roi;

    const auto& ocr_replace_num = Task.get<OcrTaskInfo>("NumberOcrReplace");
    OcrWithPreprocessImageAnalyzer::set_use_char_model(true);
    OcrWithPreprocessImageAnalyzer::set_replace(ocr_replace_num->replace_map, ocr_replace_num->replace_full);

    for (const auto& box : m_lv_flags) {
        Rect roi = box.rect.move(level_roi);
        if (roi.x + roi.width >= WindowWidthDefault) {
            continue;
        }
        OcrWithPreprocessImageAnalyzer::set_roi(roi);
        if (OcrWithPreprocessImageAnalyzer::analyze()) {
            m_oper_levels.insert(m_oper_levels.end(), std::make_move_iterator(m_ocr_result.begin()),
                                 std::make_move_iterator(m_ocr_result.end()));
        }
    }
    if (m_oper_levels.empty()) {
        return false;
    }

    //精英一位置：能为空
    m_multi_match_image_analyzer.set_task_info("OperBoxFlagElitismOne");
    m_multi_match_image_analyzer.analyze();
    m_elite_ones = m_multi_match_image_analyzer.get_result();

    //精英二位置：能为空
    m_multi_match_image_analyzer.set_task_info("OperBoxFlagElitismTwo");
    m_multi_match_image_analyzer.analyze();
    m_elite_twos = m_multi_match_image_analyzer.get_result();

    return true;
}

bool asst::OperBoxImageAnalyzer::is_near(asst::Rect rect1, asst::Rect rect2)
{
    return (std::abs(rect1.x - rect2.x) < 10 && std::abs(rect1.y - rect2.y) < 55);
}

void asst::OperBoxImageAnalyzer::draft_img()
{
    m_image_draw_oper = m_image;

    // name
    for (auto& name : m_oper_names) {
        cv::rectangle(m_image_draw_oper, make_rect<cv::Rect>(name.rect), cv::Scalar(0, 255, 0), 2);
        cv::putText(m_image_draw_oper, name.text, cv::Point(name.rect.x, name.rect.y - 10), cv::FONT_HERSHEY_SIMPLEX,
                    0.5, cv::Scalar(0, 0, 255), 2);
    }

    // level
    for (auto& level : m_oper_levels) {
        cv::rectangle(m_image_draw_oper, make_rect<cv::Rect>(level.rect), cv::Scalar(0, 255, 0), 2);
        cv::putText(m_image_draw_oper, level.text, cv::Point(level.rect.x, level.rect.y - 10), cv::FONT_HERSHEY_SIMPLEX,
                    0.5, cv::Scalar(0, 0, 255), 2);
    }
}
