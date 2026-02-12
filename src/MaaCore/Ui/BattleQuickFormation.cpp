#include "BattleQuickFormation.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Task/ProcessTask.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

asst::BattleQuickFormation::BattleQuickFormation(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain) :
    InstHelper(inst),
    m_callback(callback),
    m_task_chain(task_chain)
{
}

std::pair<int, int> asst::BattleQuickFormation::analyze_oper_level(const cv::Mat& image, asst::Rect flag)
{
    const auto& level_task = Task.get<OcrTaskInfo>("BattleQuickFormation-Level");
    const auto& elite_task = Task.get<MatchTaskInfo>("BattleQuickFormation-Elite");

    int _elite = -1;
    int _level = -1;

    // 识别精二
    Matcher elite_analyzer(image);
    elite_analyzer.set_task_info(elite_task);
    Rect roi = flag.move(elite_task->rect_move);
    elite_analyzer.set_roi(roi);
    if (!elite_analyzer.analyze()) {
        LogError << __FUNCTION__ << "| failed to analyze elite of oper";
    }
    else {
        auto elite_str = elite_analyzer.get_result().templ_name;
        elite_str = elite_str.substr(elite_str.size() - 5, 1);
        if (!utils::chars_to_number(elite_str, _elite)) {
            LogError << __FUNCTION__ << "| failed to parse elite from template:" << elite_str;
        }
    }

    // 识别等级
    RegionOCRer level_ocrer(image);
    level_ocrer.set_task_info(level_task);
    level_ocrer.set_roi(flag.move(level_task->rect_move));
    if (!level_ocrer.analyze()) {
        LogError << __FUNCTION__ << "| failed to analyze level of oper";
    }
    else if (!utils::chars_to_number(level_ocrer.get_result().text, _level)) {
        LogError << __FUNCTION__ << "| failed to parse level from text:" << level_ocrer.get_result().text;
    }

    return std::make_pair(_elite, _level);
}

std::optional<asst::BattleQuickFormation::SkillResult>
    asst::BattleQuickFormation::find_oper_skill(int skill, bool skip_level)
{
    const auto& base_task = Task.get("BattleQuickFormationSkillLevel-Base");
    const auto& check_task = Task.get("BattleQuickFormationSkillLevel-Check");
    const auto& swipe_task = Task.get("BattleQuickFormationSkillLevel-Swipe");

    cv::Mat image, roi_image;
    if (skill == 1 || skill == 2) {
        image = ctrler()->get_image();
        roi_image = make_roi(image, make_rect<cv::Rect>(base_task->roi));
        auto result = find_skill(roi_image, skill, false, skip_level);
        if (result) { // 提前找到快速返回, 否则回退到图片合并及滑动
            // 技能等级icon相对于技能rect的坐标转换回全图坐标
            return SkillResult {
                VisionHelper::correct_rect(
                    base_task->roi.move(result->first).move(check_task->rect_move),
                    Rect { 0, 0, WindowWidthDefault, WindowHeightDefault }),
                result->second,
            };
        }
    }
    else {
        ProcessTask(m_callback, m_inst, m_task_chain).set_tasks({ "BattleQuickFormationSkill-SwipeToTheDown" }).run();
        image = ctrler()->get_image();
        roi_image = make_roi(image, make_rect<cv::Rect>(base_task->roi));
        auto result = find_skill(roi_image, skill, true, skip_level);
        if (result) { // 提前找到快速返回, 否则回退到图片合并及滑动
            return SkillResult {
                VisionHelper::correct_rect(
                    base_task->roi.move(result->first).move(check_task->rect_move),
                    Rect { 0, 0, WindowWidthDefault, WindowHeightDefault }),
                result->second,
            };
        }
        ProcessTask(m_callback, m_inst, m_task_chain).set_tasks({ "BattleQuickFormationSkill-SwipeToTheUp" }).run();
        image = ctrler()->get_image(); // 一般不会走到这里, 翻回顶部走通用逻辑
        roi_image = make_roi(image, make_rect<cv::Rect>(base_task->roi));
    }

    int retry = 0;
    int last_y = -1;
    cv::Mat stitched_image = roi_image;
    while (!need_exit() && retry < 3) {
        ctrler()->swipe(swipe_task->specific_rect, swipe_task->rect_move, 300, true, 3.7, 0.1);
        sleep(swipe_task->post_delay);

        image = ctrler()->get_image();
        auto roi_image_new = make_roi(image, make_rect<cv::Rect>(base_task->roi));

        // 使用模板匹配检测重叠区域
        Matcher match(stitched_image);
        match.set_templ(make_roi(roi_image_new, cv::Rect { 0, 0, roi_image_new.cols, 30 }));
        match.set_method(MatchMethod::Ccoeff);
        match.set_threshold(0.8);
        if (!match.analyze()) {
            retry++;
            continue;
        }

        int current_y = match.get_result().rect.y;
        if (last_y != -1 && std::abs(current_y - last_y) < 5) { // 检测是否已经滑动到末端
            LogDebug << __FUNCTION__ << "| Reached the end of skill list, y offset unchanged: " << current_y;
            break;
        }

        last_y = current_y;

        // 即时拼接图片：保留新图片中未重叠的部分
        int overlap_height = stitched_image.rows - current_y;
        if (overlap_height > 0 && overlap_height < roi_image_new.rows) {
            cv::Mat non_overlap_part =
                roi_image_new(cv::Rect(0, overlap_height, roi_image_new.cols, roi_image_new.rows - overlap_height));

            cv::vconcat(stitched_image, non_overlap_part, stitched_image); // 拼接到已有图片
            LogDebug << __FUNCTION__ << "| Stitched image updated, current size: " << stitched_image.cols << "x"
                     << stitched_image.rows;
        }
        else {
            LogWarn << __FUNCTION__ << "| Invalid overlap detected, overlap_height: " << overlap_height;
            retry++;
            continue;
        }

        retry = 0; // 成功拼接，重置重试计数
        // 短路检测, 在已拼接的图片上尝试查找目标技能
        auto result = find_skill(stitched_image, skill, false, skip_level);
        if (result) {
            // 拼接图片中的技能等级icon坐标转换回全图的icon坐标
            Rect rect { result->first.x,
                        result->first.y + base_task->roi.y - last_y,
                        result->first.width,
                        result->first.height };

            // 偏移为技能范围rect, 并根据图片大小修正
            return SkillResult {
                VisionHelper::correct_rect(
                    rect.move(check_task->rect_move),
                    Rect { 0, 0, WindowWidthDefault, WindowHeightDefault }),
                result->second,
            };
        }
    }

    auto result = find_skill(stitched_image, skill, false, skip_level);
    if (result) {
        // 拼接图片中的技能等级icon坐标转换回全图的icon坐标
        Rect rect { result->first.x,
                    result->first.y + base_task->roi.y - last_y,
                    result->first.width,
                    result->first.height };

        // 偏移为技能范围rect, 并根据图片大小修正
        return SkillResult {
            VisionHelper::correct_rect(
                rect.move(check_task->rect_move),
                Rect { 0, 0, WindowWidthDefault, WindowHeightDefault }),
            result->second,
        };
    }

    return std::nullopt;
}

std::optional<std::pair<asst::Rect, int>>
    asst::BattleQuickFormation::find_skill(const cv::Mat& image, int skill, bool reverse, bool skip_level)
{
    const auto& base_task = Task.get("BattleQuickFormationSkillLevel-Base");
    const auto& check_task = Task.get("BattleQuickFormationSkillLevel-Check");
    const auto& ocr_task = Task.get("BattleQuickFormationSkillLevel-OCR");

    // 匹配并识别技能等级, 输入技能闪电icon的rect
    const auto analyze_level = [&](asst::Rect rect) -> std::optional<std::pair<asst::Rect, int>> {
        if (skip_level) {
            return std::make_pair(rect.move(base_task->rect_move), 0);
        }
        Matcher level_matcher(image);
        level_matcher.set_task_info(check_task);
        level_matcher.set_roi(rect.move(base_task->rect_move));
        if (level_matcher.analyze()) {
            int level;
            auto pos = level_matcher.get_result().templ_name.find_last_of('-');
            if (pos != std::string::npos &&
                utils::chars_to_number<int>(level_matcher.get_result().templ_name.substr(pos + 1, 2), level)) {
                LogDebug << __FUNCTION__ << "| skill" << skill << "level:" << level;
                return std::make_pair(rect.move(base_task->rect_move), level);
            }
            else {
                LogError << __FUNCTION__ << "| skill" << skill
                         << "level parsing failed from template name:" << level_matcher.get_result().templ_name;
            }
        }

        RegionOCRer ocrer(image);
        ocrer.set_task_info(ocr_task);
        ocrer.set_roi(rect.move(ocr_task->roi));
        if (ocrer.analyze()) {
            int level;
            if (utils::chars_to_number<int>(ocrer.get_result().text, level)) {
                LogDebug << __FUNCTION__ << "| skill" << skill << "level:" << level;
                return std::make_pair(rect.move(ocr_task->roi), level);
            }
            else {
                LogError << __FUNCTION__ << "| skill" << skill
                         << "level parsing failed from OCR text:" << ocrer.get_result().text;
            }
        }

        LogError << __FUNCTION__ << "| skill" << skill << "level not found";
        utils::save_debug_image(image, utils::path("debug/copilot/formation"), true);
        return std::nullopt;
    };

    int index = 0;

    MultiMatcher matcher(image);
    matcher.set_templ("BattleQuickFormationSkillLevel-Base.png");
    matcher.set_threshold(0.8);
    matcher.set_method(MatchMethod::Ccoeff);
    if (matcher.analyze()) { // 匹配成功, 检查技能等级
        if (reverse && skill == 3) {
            auto results = matcher.get_result();
            if (!results.empty()) {
                return analyze_level(results.back().rect);
            }
        }
        else if (reverse) {
            LogError << __FUNCTION__ << "| Only skill 3 can be reverse matched";
        }

        for (const auto& match : matcher.get_result()) {
            if (need_exit() || ++index != skill) {
                continue;
            }
            return analyze_level(match.rect);
        }
    }

    return std::nullopt;
}
