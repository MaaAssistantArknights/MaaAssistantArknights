#include "OperBoxAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/BestMatcher.h"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"
#include "Vision/TemplDetOCRer.h"

bool asst::OperBoxAnalyzer::analyze()
{
    LogTraceFunction;

    m_result.clear();

    bool ret = analyzer_oper_box();

    save_img(utils::path("debug") / utils::path("oper"));

    return ret;
}

int asst::OperBoxAnalyzer::level_num(const std::string& level)
{
    if (level.empty() || !ranges::all_of(level, [](const char& c) -> bool { return std::isdigit(c); })) {
        return 1;
    }
    return std::stoi(level);
}

bool asst::OperBoxAnalyzer::analyzer_oper_box()
{
    LogTraceFunction;

    if (!opers_analyze()) {
        return false;
    }

    if (!level_analyze()) {
        return false;
    }

    if (!elite_analyze()) {
        return false;
    }

    if (!potential_analyze()) {
        return false;
    }

    sort_by_horizontal_(m_result);

    return !m_result.empty();
}

bool asst::OperBoxAnalyzer::opers_analyze()
{
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");

    TemplDetOCRer oper_name_analyzer(m_image);

    oper_name_analyzer.set_task_info("OperBoxFlagLV", "OperBoxNameOCR");
    oper_name_analyzer.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
    const auto& all_opers = BattleData.get_all_oper_names();
    oper_name_analyzer.set_required(std::vector(all_opers.begin(), all_opers.end()));

    auto oper_result_opt = oper_name_analyzer.analyze();
    if (!oper_result_opt) {
        return false;
    }

    for (const auto& oper : *oper_result_opt) {
        const Rect& flag_rect = oper.flag_rect;
        const std::string& name = oper.text;

        OperBoxInfo box;
        box.id = BattleData.get_id(name);
        box.name = name;
        box.rarity = BattleData.get_rarity(name);

        box.rect = flag_rect;
        box.own = true;

#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(flag_rect), cv::Scalar(0, 255, 0), 2);
#endif

        m_result.emplace_back(std::move(box));
    }
    return true;
}

// 识别等级
bool asst::OperBoxAnalyzer::level_analyze()
{
    RegionOCRer level_analyzer(m_image);
    level_analyzer.set_task_info("OperBoxLevelOCR");
    const Rect level_roi = Task.get<OcrTaskInfo>("OperBoxLevelOCR")->roi;

    const auto& ocr_replace_num = Task.get<OcrTaskInfo>("NumberOcrReplace");
    level_analyzer.set_replace(ocr_replace_num->replace_map, ocr_replace_num->replace_full);

    for (auto& box : m_result) {
        Rect roi = box.rect.move(level_roi);
        if (roi.x < 0) {
            // 等级在lv的左,lv的识别框x该右移
            Log.error("level roi", roi, "is out of range");
            return false;
        }
        level_analyzer.set_roi(roi);
        if (!level_analyzer.analyze()) {
            box.level = 1;
            continue;
        }
        const auto& ocr_result = level_analyzer.get_result();
        const std::string& level = ocr_result.text;
        box.level = level_num(level);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(ocr_result.rect), cv::Scalar(0, 255, 0), 1);
        cv::putText(m_image_draw, level, cv::Point(roi.x, roi.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                    cv::Scalar(0, 0, 255), 2);
#endif // ASST_DEBUG}
    }
    return true;
}

// 识别精英度
bool asst::OperBoxAnalyzer::elite_analyze()
{
    const std::string task_name = "OperBoxFlagElite";
    const Rect elite_roi = Task.get(task_name)->roi;
    BestMatcher elite_analyzer(m_image);
    elite_analyzer.set_task_info(task_name);
    elite_analyzer.append_templ("OperBoxFlagElite1.png");
    elite_analyzer.append_templ("OperBoxFlagElite2.png");
    for (auto& box : m_result) {
        Rect roi = box.rect.move(elite_roi);
        if (roi.x < 0) {
            // 等级在lv的左,lv的识别框x该右移
            Log.error("elite roi", roi, "is out of range");
            return false;
        }
        elite_analyzer.set_roi(roi);
        auto elite_result_opt = elite_analyzer.analyze();
        if (!elite_result_opt) {
            box.elite = 0;
            continue;
        }
        std::string elite = elite_result_opt->templ_info.name.substr(task_name.size(), 1);
        box.elite = std::stoi(elite);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(roi), cv::Scalar(0, 255, 0), 1);
#endif // ASST_DEBUG
    }
    return true;
}

// 识别潜能
bool asst::OperBoxAnalyzer::potential_analyze()
{
    const std::string task_name_p = "OperBoxPotential";
    const Rect potential_roi = Task.get(task_name_p)->roi;
    BestMatcher potential_analyzer(m_image);
    potential_analyzer.set_task_info(task_name_p);
    for (int i = 1; i < 6; i++) {
        std::string potential_temp_name = task_name_p + std::to_string(i) + ".png";
        potential_analyzer.append_templ(potential_temp_name);
    }

    for (auto& box : m_result) {
        Rect roi = box.rect.move(potential_roi);

        potential_analyzer.set_roi(roi);
        auto potential_result_opt = potential_analyzer.analyze();
        if (!potential_result_opt) {
            box.potential = 0;
            continue;
        }
        std::string potential = potential_result_opt->templ_info.name.substr(task_name_p.size(), 1);
        box.potential = std::stoi(potential);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(roi), cv::Scalar(0, 255, 0), 1);
#endif // ASST_DEBUG
    }
    return true;
}
