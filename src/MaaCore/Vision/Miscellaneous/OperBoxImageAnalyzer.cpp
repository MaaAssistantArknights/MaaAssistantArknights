#include "OperBoxImageAnalyzer.h"

#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Vision/BestMatcher.h"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/OperNameAnalyzer.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"
#include "Vision/TemplDetOCRer.h"

bool asst::OperBoxImageAnalyzer::analyze()
{
    LogTraceFunction;

    m_result.clear();

    bool ret = analyzer_oper_box();
    if (m_result.size() != 16 && m_result.size() != 14) { // 完整的一页是14或16个，有可能是识别错了
        save_img(utils::path("debug") / utils::path("oper"));
    }

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

    return !m_result.empty();
}

bool asst::OperBoxImageAnalyzer::opers_analyze()
{
    const auto& name_task = Task.get("OperBoxNameOCR");
    const auto& params = Task.get("OperBoxNameOCR")->special_params;
    const auto& all_opers = BattleData.get_all_oper_names();
    const auto& analyze_task = [&](const std::string& task,
                                   const asst::Rect& roi) -> std::optional<TemplDetOCRer::ResultsVec> {
        MultiMatcher matcher(m_image);
        matcher.set_task_info(task);
        matcher.set_roi(roi);
        if (!matcher.analyze()) {
            return std::nullopt;
        }
        asst::TemplDetOCRer::ResultsVec list;
        for (const auto& flag : matcher.get_result()) {
            OperNameAnalyzer name_analyzer(m_image);
            name_analyzer.set_task_info(name_task);
            name_analyzer.set_required(std::vector(all_opers.begin(), all_opers.end()));
            name_analyzer.set_roi(flag.rect.move(name_task->rect_move));
            name_analyzer.set_bin_threshold(params[0]);
            name_analyzer.set_bin_expansion(params[1]);
            name_analyzer.set_bin_trim_threshold(params[2], params[3]);
            name_analyzer.set_bottom_line_height(params[4]);
            name_analyzer.set_width_threshold(params[5]);
            [[maybe_unused]] cv::Mat debug_img = make_roi(m_image, flag.rect.move(name_task->rect_move));
            if (auto ocr_opt = name_analyzer.analyze()) {
                asst::TemplDetOCRer::Result ocr { .flag_rect = flag.rect, .flag_score = flag.score };
                ocr.rect = ocr_opt->rect;
                ocr.text = ocr_opt->text;
                ocr.score = ocr_opt->score;
                list.emplace_back(std::move(ocr));
            }
            else {
                Log.error("OperNameAnalyzer analyze failed");
            }
        }
        if (list.empty()) {
            return std::nullopt;
        }
        return list;
    };

    TemplDetOCRer::ResultsVec results;

    Rect roi_top = Task.get("OperBoxFlagRoleTopROI")->roi;
    Rect roi_bottom = Task.get("OperBoxFlagRoleBottomROI")->roi;

    for (int i = 1; i < 10; ++i) {
        if (auto top_result_opt = analyze_task("OperBoxFlagRole" + std::to_string(i), roi_top)) {
            ranges::move(*top_result_opt, std::back_inserter(results));
        }

        if (auto bottom_result_opt = analyze_task("OperBoxFlagRole" + std::to_string(i), roi_bottom)) {
            ranges::move(*bottom_result_opt, std::back_inserter(results));
        }
    }

    if (results.empty()) {
        return false;
    }
    results = NMS(std::move(results));
    sort_by_horizontal_(results);

    for (const auto& oper : results) {
        const Rect& flag_rect = oper.flag_rect;
        const std::string& name = oper.text;

        OperBoxInfo box;
        box.id = BattleData.get_id(name);
        box.name = name;
        box.rarity = BattleData.get_rarity(name);

        box.rect = flag_rect;
        box.own = true;

#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(flag_rect), cv::Scalar(0, 255, 0), 1);
        cv::putText(
            m_image_draw,
            std::to_string(oper.flag_score),
            cv::Point(flag_rect.x, flag_rect.y - 10),
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,
            cv::Scalar(0, 0, 255),
            1);
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(oper.rect), cv::Scalar(0, 255, 0), 1);
#endif

        m_result.emplace_back(std::move(box));
    }
    return true;
}

// 识别等级
bool asst::OperBoxImageAnalyzer::level_analyze()
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
        cv::putText(
            m_image_draw,
            level,
            cv::Point(roi.x, roi.y - 10),
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,
            cv::Scalar(0, 0, 255),
            2);
#endif // ASST_DEBUG}
    }
    return true;
}

// 识别精英度
bool asst::OperBoxImageAnalyzer::elite_analyze()
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
        if (!elite_analyzer.analyze()) {
            box.elite = 0;
            continue;
        }
        const auto& elite_templ = elite_analyzer.get_result();
        std::string elite = elite_templ.templ_info.name.substr(task_name.size(), 1);
        box.elite = std::stoi(elite);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(roi), cv::Scalar(0, 255, 0), 1);
        cv::putText(
            m_image_draw,
            std::to_string(box.elite),
            cv::Point(roi.x, roi.y - 10),
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,
            cv::Scalar(0, 0, 255),
            2);
#endif // ASST_DEBUG
    }
    return true;
}

// 识别潜能
bool asst::OperBoxImageAnalyzer::potential_analyze()
{
    const std::string task_name_p = "OperBoxPotential";
    const Rect potential_roi = Task.get(task_name_p)->roi;
    BestMatcher potential_analyzer(m_image);
    potential_analyzer.set_task_info(task_name_p);
    for (int i = 2; i <= 6; i++) {
        std::string potential_temp_name = task_name_p + std::to_string(i) + ".png";
        potential_analyzer.append_templ(potential_temp_name);
    }

    for (auto& box : m_result) {
        Rect roi = box.rect.move(potential_roi);

        potential_analyzer.set_roi(roi);
        if (!potential_analyzer.analyze()) {
            box.potential = 1;
            continue;
        }
        const auto& poten_templ = potential_analyzer.get_result();
        std::string potential = poten_templ.templ_info.name.substr(task_name_p.size(), 1);
        box.potential = std::stoi(potential);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(roi), cv::Scalar(0, 255, 0), 1);
        cv::putText(
            m_image_draw,
            std::to_string(box.potential),
            cv::Point(roi.x, roi.y - 10),
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,
            cv::Scalar(0, 0, 255),
            2);
#endif // ASST_DEBUG
    }
    return true;
}
