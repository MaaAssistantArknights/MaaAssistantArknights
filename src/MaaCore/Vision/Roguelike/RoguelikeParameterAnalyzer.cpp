#include "RoguelikeParameterAnalyzer.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/ImageIo.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

/* bool asst::RoguelikeParameterAnalyzer::analyze()
{
    LogTraceFunction;
    return true;
}*/

int asst::RoguelikeParameterAnalyzer::get_number(const cv::Mat& image, const std::string& task_name)
{
    LogTraceFunction;

    int val = 0;
    OCRer analyzer(image);
    analyzer.set_task_info(task_name);
    analyzer.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);
    analyzer.set_use_char_model(true);

    if (!analyzer.analyze()) {
        return -1;
    }

    return utils::chars_to_number(analyzer.get_result().front().text, val) ? val : -1;
}

int asst::RoguelikeParameterAnalyzer::update_hp(const cv::Mat& image)
{
    LogTraceFunction;

    Matcher matcher(image);
    matcher.set_task_info("Roguelike@HpFlag");
    if (!matcher.analyze()) {
        Log.info("Not found HpFlag");
        return -1;
    }

    auto task = Task.get<OcrTaskInfo>("Roguelike@HpRecognition");
    std::vector<std::pair<std::string, std::string>> merged_map;
    merged_map.insert(merged_map.end(), task->replace_map.begin(), task->replace_map.end());
    merged_map.emplace_back("(.*)/.*", "$1");

    auto roi_image = make_roi(image, task->roi).clone();
    cv::Mat r_channel;
    cv::extractChannel(roi_image, r_channel, 2);
    cv::Mat mask;
    cv::threshold(r_channel, mask, 50, 255, cv::THRESH_BINARY);
    cv::Mat inv_mask;
    cv::bitwise_not(mask, inv_mask);
    roi_image.setTo(cv::Scalar(0, 0, 0), mask);

    RegionOCRer analyzer(roi_image);
    analyzer.set_replace(merged_map);
    analyzer.set_use_char_model(true);
    analyzer.set_bin_threshold(60); // 血量没有红色通道，虽然它看着很明显，但实际上在灰度中只有 2/3

    auto res_vec_opt = analyzer.analyze();
    if (!res_vec_opt) {
        return -1;
    }

    int hp_val;
    return utils::chars_to_number(res_vec_opt->text, hp_val) ? hp_val : -1;
}

// ============================================

int asst::RoguelikeParameterAnalyzer::update_hope(const cv::Mat& image, const std::string& theme)
{
    LogTraceFunction;

    // 先识别 当前◀希望◀希望上限 那两个三角形标志
    asst::Matcher matcher_left(image);
    matcher_left.set_task_info(theme + "@Roguelike@HopeLeftFlag");
    auto match_res_opt_left = matcher_left.analyze();
    if (!match_res_opt_left) {
        Log.error(__FUNCTION__, "| Not found HopeLeftFlag");
        return -1;
    }
    asst::Matcher matcher_right(image);
    matcher_right.set_task_info(theme + "@Roguelike@HopeRightFlag");
    auto match_res_opt_right = matcher_right.analyze();
    if (!match_res_opt_right) {
        Log.error(__FUNCTION__, "| Not found HopeRightFlag");
        return -1;
    }

    int hope_val;
    asst::OCRer analyzer(image);
    analyzer.set_task_info(theme + "@Roguelike@HopeRecognition");
    auto task_roi = Task.get<OcrTaskInfo>(theme + "@Roguelike@HopeRecognition")->roi;
    // roi即为两个标志之间的区域
    auto roi = Rect(
        match_res_opt_left->rect.x + match_res_opt_left->rect.width + task_roi.x,
        match_res_opt_left->rect.y + task_roi.y,
        match_res_opt_right->rect.x - (match_res_opt_left->rect.x + match_res_opt_left->rect.width) + task_roi.width,
        task_roi.height);
    analyzer.set_roi(roi);

    auto res_vec_opt = analyzer.analyze();
    if (!res_vec_opt) {
        return -1;
    }
    return utils::chars_to_number(res_vec_opt->front().text, hope_val) ? hope_val : 0;
}

int asst::RoguelikeParameterAnalyzer::update_ingot(const cv::Mat& image, const std::string& theme)
{
    LogTraceFunction;

    Matcher matcher(image);
    matcher.set_task_info(theme + "@Roguelike@IngotFlag");
    auto match_res_opt = matcher.analyze();
    if (!match_res_opt) {
        Log.error(__FUNCTION__, "| Not found IngotFlag");
        return -1;
    }

    int ingot_val;
    asst::OCRer analyzer(image);
    analyzer.set_task_info(theme + "@Roguelike@IngotRecognition");

    auto res_vec_opt = analyzer.analyze();
    if (!res_vec_opt) {
        return -1;
    }
    return utils::chars_to_number(res_vec_opt->front().text, ingot_val) ? ingot_val : 0;
}

// =============================================

int asst::RoguelikeParameterAnalyzer::update_chaos(const cv::Mat& image, const std::string& theme)
{
    if (theme != "Sami") {
        Log.error(__FUNCTION__, "| update_chaos only support Sami theme");
        return -1;
    }
    return get_number(image, theme + "@Roguelike@SpecialValRecognition");
}

int asst::RoguelikeParameterAnalyzer::update_idea_count(const cv::Mat& image, const std::string& theme)
{
    if (theme != "Sarkaz") {
        Log.error(__FUNCTION__, "| update_idea_count only support Sarkaz theme");
        return -1;
    }
    return get_number(image, theme + "@Roguelike@SpecialValRecognition");
}

int asst::RoguelikeParameterAnalyzer::update_ticket_count(const cv::Mat& image, const std::string& theme)
{
    if (theme != "JieGarden") {
        Log.error(__FUNCTION__, "| update_ticket_count only support JieGarden theme");
        return -1;
    }
    return get_number(image, theme + "@Roguelike@TicketRecognition");
}
