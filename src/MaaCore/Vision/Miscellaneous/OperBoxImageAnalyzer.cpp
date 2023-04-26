#include "OperBoxImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/BestMatchImageAnalyzer.h"
#include "Vision/MatchImageAnalyzer.h"
#include "Vision/MultiMatchImageAnalyzer.h"
#include "Vision/OcrWithFlagTemplImageAnalyzer.h"

bool asst::OperBoxImageAnalyzer::analyze()
{
    LogTraceFunction;

    m_result.clear();

    bool ret = analyzer_oper_box();

    save_img(utils::path("debug") / utils::path("oper"));

    return ret;
}

int asst::OperBoxImageAnalyzer::level_num(const std::string& level)
{
    if (level.empty() || !ranges::all_of(level, [](const char& c) -> bool { return std::isdigit(c); })) {
        return 1;
    }
    return std::stoi(level);
}

bool asst::OperBoxImageAnalyzer::analyzer_oper_box()
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

    sort_();

    return !m_result.empty();
}

bool asst::OperBoxImageAnalyzer::opers_analyze()
{
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");

    OcrWithFlagTemplImageAnalyzer oper_name_analyzer(m_image);

    oper_name_analyzer.set_task_info("OperBoxFlagLV", "OperBoxNameOCR");
    oper_name_analyzer.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
    const auto& all_opers = BattleData.get_all_oper_names();
    oper_name_analyzer.set_required(std::vector(all_opers.begin(), all_opers.end()));
    if (!oper_name_analyzer.analyze()) {
        return false;
    }

    size_t size = oper_name_analyzer.get_flag_result().size();
    const auto& flag_list = oper_name_analyzer.get_flag_result();
    const auto& name_list = oper_name_analyzer.get_result();
    for (size_t i = 0; i != size; ++i) {
        const auto& flag_rect = flag_list[i];
        const auto& name_tr = name_list[i];

        const std::string& name = name_tr.text;

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
bool asst::OperBoxImageAnalyzer::level_analyze()
{
    OcrWithPreprocessImageAnalyzer level_analyzer(m_image);
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
        const auto& ocr_result = level_analyzer.get_result().front();
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
bool asst::OperBoxImageAnalyzer::elite_analyze()
{
    const std::string task_name = "OperBoxFlagElite";
    const Rect elite_roi = Task.get(task_name)->roi;
    BestMatchImageAnalyzer elite_analyzer(m_image);
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
        if (!elite_analyzer.analyze()) {
            box.elite = 0;
            continue;
        }
        const auto& elite_templ = elite_analyzer.get_result();
        std::string elite = elite_templ.name.substr(task_name.size(), 1);
        box.elite = std::stoi(elite);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(roi), cv::Scalar(0, 255, 0), 1);
#endif // ASST_DEBUG
    }
    return true;
}

// 识别潜能
bool asst::OperBoxImageAnalyzer::potential_analyze()
{
    const std::string task_name_p = "OperBoxPotential";
    const Rect potential_roi = Task.get(task_name_p)->roi;
    BestMatchImageAnalyzer potential_analyzer(m_image);
    potential_analyzer.set_task_info(task_name_p);
    for (int i = 1; i < 6; i++) {
        std::string potential_temp_name = task_name_p + std::to_string(i) + ".png";
        potential_analyzer.append_templ(potential_temp_name);
    }

    for (auto& box : m_result) {
        Rect roi = box.rect.move(potential_roi);

        potential_analyzer.set_roi(roi);
        if (!potential_analyzer.analyze()) {
            box.potential = 0;
            continue;
        }
        const auto& poten_templ = potential_analyzer.get_result();
        std::string potential = poten_templ.name.substr(task_name_p.size(), 1);
        box.potential = std::stoi(potential);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(roi), cv::Scalar(0, 255, 0), 1);
#endif // ASST_DEBUG
    }
    return true;
}

void asst::OperBoxImageAnalyzer::sort_()
{
    // 按位置排个序
    ranges::sort(m_result, [](const OperBoxInfo& lhs, const OperBoxInfo& rhs) -> bool {
        if (std::abs(lhs.rect.y - rhs.rect.y) < 5) { // y差距较小则理解为是同一排的，按x排序
            return lhs.rect.x < rhs.rect.x;
        }
        else {
            return lhs.rect.y < rhs.rect.y;
        }
    });
}
