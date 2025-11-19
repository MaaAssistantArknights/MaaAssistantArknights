#include "RoguelikeCoppersAnalyzer.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

#include <string_view>
#include <utility>

// RoguelikeCoppersAnalyzer 类用于分析界园肉鸽的通宝（coppers）
// 包括拾取通宝和交换通宝的识别和处理

namespace
{
// 定义任务名称常量，用于从配置中获取相应的任务信息
constexpr std::string_view kPickupMatcherTask = "JieGarden@Roguelike@GetDropSwitch";
constexpr std::string_view kPickupNameTask = "JieGarden@Roguelike@CoppersAnalyzer-PickupNameOCR";
constexpr std::string_view kExchangeNameTask = "JieGarden@Roguelike@CoppersAnalyzer-ExchangeNameOCR";
constexpr std::string_view kCastTask = "JieGarden@Roguelike@CoppersAnalyzer-CastOCR";
constexpr std::string_view kLeftColumnMatcherTask = "JieGarden@Roguelike@CoppersAnalyzer-LeftType";
constexpr std::string_view kMiddleColumnMatcherTask = "JieGarden@Roguelike@CoppersAnalyzer-Type";
constexpr std::string_view kRightColumnMatcherTask = "JieGarden@Roguelike@CoppersAnalyzer-RightType";

// 将 ColumnRole 枚举转换为字符串，用于日志输出
inline std::string to_string(asst::RoguelikeCoppersAnalyzer::ColumnRole role)
{
    using Role = asst::RoguelikeCoppersAnalyzer::ColumnRole;
    switch (role) {
    case Role::Leftmost:
        return "Leftmost";
    case Role::Middle:
        return "Middle";
    case Role::Rightmost:
        return "Rightmost";
    }
    return "Unknown";
}
}

namespace asst
{

// 分析拾取的通宝（coppers），使用模板匹配和OCR识别通宝名称
bool RoguelikeCoppersAnalyzer::analyze_pickup()
{
    LogTraceFunction;

    const auto name_task = Task.get<OcrTaskInfo>(kPickupNameTask);
    if (!name_task) {
        Log.error(__FUNCTION__, "| failed to load pickup name OCR task");
        return false;
    }

    return analyze_internal(std::string(kPickupMatcherTask), *name_task, nullptr, SortStrategy::Horizontal);
}

// 分析指定列的通宝（coppers），支持检测是否已投出（cast）
// role: 列的位置（最左、中间、最右）
// detect_cast: 是否检测投出状态
bool RoguelikeCoppersAnalyzer::analyze_column(ColumnRole role, bool detect_cast)
{
    LogTraceFunction;

    const auto name_task = Task.get<OcrTaskInfo>(kExchangeNameTask);
    if (!name_task) {
        Log.error(__FUNCTION__, "| failed to load exchange name OCR task");
        return false;
    }

    const auto cast_task = detect_cast ? Task.get<OcrTaskInfo>(kCastTask) : nullptr;
    if (detect_cast && !cast_task) {
        Log.error(__FUNCTION__, "| failed to load cast OCR task");
        return false;
    }

    std::string matcher_task;
    switch (role) {
    case ColumnRole::Leftmost:
        matcher_task.assign(kLeftColumnMatcherTask);
        break;
    case ColumnRole::Middle:
        matcher_task.assign(kMiddleColumnMatcherTask);
        break;
    case ColumnRole::Rightmost:
        matcher_task.assign(kRightColumnMatcherTask);
        break;
    }

    const bool analyzed = analyze_internal(matcher_task, *name_task, cast_task.get(), SortStrategy::Vertical);
    if (!analyzed) {
        Log.error(__FUNCTION__, "| failed to analyze column ", to_string(role));
    }
    return analyzed;
}

// 内部分析函数，执行模板匹配和OCR识别
// matcher_task: 模板匹配任务名称
// name_task: OCR任务信息，用于识别通宝名称
// cast_task: 可选的OCR任务，用于检测投出状态
// sort_strategy: 排序策略（水平或垂直）
bool RoguelikeCoppersAnalyzer::analyze_internal(
    const std::string& matcher_task,
    const OcrTaskInfo& name_task,
    const OcrTaskInfo* cast_task,
    SortStrategy sort_strategy)
{
    // 检查图像是否为空
    if (m_image.empty()) {
        Log.error(__FUNCTION__, "| empty image for matcher task ", matcher_task);
        return false;
    }

    // 执行模板匹配
    MultiMatcher matcher(m_image);
    matcher.set_task_info(matcher_task);

    if (!matcher.analyze()) {
        Log.error(__FUNCTION__, "| matcher analyze failed for task ", matcher_task);
        return false;
    }

    auto match_results = matcher.get_result();
    if (match_results.empty()) {
        Log.error(__FUNCTION__, "| matcher returned empty result for task ", matcher_task);
        return false;
    }

    // 根据策略排序匹配结果
    if (sort_strategy == SortStrategy::Horizontal) {
        sort_by_horizontal_(match_results);
    }
    else {
        sort_by_vertical_(match_results);
    }

    // 初始化OCR识别器
    RegionOCRer name_ocr(m_image);
    name_ocr.set_replace(name_task.replace_map, name_task.replace_full);

    RegionOCRer cast_ocr(m_image);
    if (cast_task != nullptr) {
        cast_ocr.set_replace(cast_task->replace_map, cast_task->replace_full);
    }

    // 清空之前的检测结果
    m_detections.clear();
    m_metrics = {};

    // 预分配空间以避免多次内存分配
    m_detections.reserve(match_results.size());

    // 遍历每个匹配结果，进行OCR识别
    for (const auto& match_result : match_results) {
        // 计算名称识别区域
        const Rect name_roi = match_result.rect.move(name_task.roi);
        name_ocr.set_roi(name_roi);
        if (!name_ocr.analyze()) {
            Log.error(__FUNCTION__, "| failed to recognize copper name at", name_roi.to_string());
            continue;
        }

        const auto& name_result = name_ocr.get_result();
        if (name_result.text.empty()) {
            Log.error(__FUNCTION__, "| empty copper name at", name_roi.to_string());
            continue;
        }

        // 创建检测结果对象
        CopperDetection detection {};
        detection.match_rect = match_result.rect;
        detection.name_roi = name_roi;
        detection.click_point = Point(
            match_result.rect.x + match_result.rect.width / 2,
            match_result.rect.y + match_result.rect.height / 2);
        detection.templ_name = match_result.templ_name;
        detection.name = name_result.text;
        detection.name_score = name_result.score;

        // 如果需要检测投出状态
        if (cast_task != nullptr) {
            detection.cast_roi = match_result.rect.move(cast_task->roi);
            cast_ocr.set_roi(detection.cast_roi);
            if (cast_ocr.analyze()) {
                const auto& cast_result = cast_ocr.get_result();
                detection.cast_score = cast_result.score;
                detection.cast_recognized = !cast_result.text.empty();
                detection.is_cast = cast_result.text.find("已投出") != std::string::npos;
            }
        }

        Log.info(__FUNCTION__, "| detection", detection.name, detection.match_rect);
        m_detections.emplace_back(std::move(detection));
    }

    // 检查是否有有效的检测结果
    if (m_detections.empty()) {
        Log.error(__FUNCTION__, "| no valid detections for task ", matcher_task);
        return false;
    }

    // 计算垂直排序的度量信息
    if (sort_strategy == SortStrategy::Vertical) {
        m_metrics.m_x = m_detections.front().match_rect.x;
        m_metrics.m_y = m_detections.front().match_rect.y;
        if (m_detections.size() > 1U) {
            m_metrics.row_offset = m_detections[1].match_rect.y - m_detections[0].match_rect.y;
        }
    }
    else {
        m_metrics = {};
    }

    return true;
}

} // namespace asst
