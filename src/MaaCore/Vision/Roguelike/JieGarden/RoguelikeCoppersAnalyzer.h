#pragma once

#include "Vision/VisionHelper.h"

#include <string>
#include <vector>

namespace asst
{
struct OcrTaskInfo;

// RoguelikeCoppersAnalyzer 类用于分析界园肉鸽的通宝（coppers）
class RoguelikeCoppersAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    ~RoguelikeCoppersAnalyzer() override = default;

    // 列度量信息，用于记录通宝列表的坐标基准点
    struct ColumnMetrics
    {
        int m_x = 0;        // 第一列节点的横坐标
        int m_y = 0;        // 第一行节点的纵坐标
        int row_offset = 0; // 两行节点之间的距离
    };

    // 列位置枚举，表示通宝列表中的列位置
    enum class ColumnRole
    {
        Leftmost, // 最左侧列（通常是新拾取的通宝）
        Middle,   // 中间列
        Rightmost // 最右侧列
    };

    // 通宝检测结果结构体，包含识别到的通宝信息
    struct CopperDetection
    {
        Rect match_rect;              // 模板匹配的矩形区域
        Rect name_roi;                // 名称OCR识别区域
        Rect cast_roi;                // 已投出状态识别区域
        Point click_point;            // 点击坐标点
        std::string templ_name;       // 模板文件名
        std::string name;             // 识别到的通宝名称
        double name_score = 0.0;      // 名称识别置信度
        double cast_score = 0.0;      // 已投出状态识别置信度
        bool cast_recognized = false; // 是否识别到已投出状态
        bool is_cast = false;         // 是否已投出
    };

    // 分析拾取界面中的通宝（水平排列）
    bool analyze_pickup();

    // 分析指定列的通宝（垂直排列），可选择是否检测已投出状态
    bool analyze_column(ColumnRole role, bool detect_cast = true);

    // 获取所有检测到的通宝结果
    const std::vector<CopperDetection>& get_detections() const noexcept { return m_detections; }

    // 获取列的度量信息
    const ColumnMetrics& get_column_metrics() const noexcept { return m_metrics; }

private:
    // 排序策略枚举
    enum class SortStrategy
    {
        Horizontal, // 水平排序（用于拾取界面）
        Vertical    // 垂直排序（用于交换界面）
    };

    // 内部分析函数，执行模板匹配和OCR识别
    bool analyze_internal(
        const std::string& matcher_task,       // 模板匹配任务名称
        const OcrTaskInfo& name_task,          // 名称OCR任务信息
        const OcrTaskInfo* cast_task,          // 已投出状态OCR任务信息（可选）
        SortStrategy sort_strategy);           // 排序策略

    std::vector<CopperDetection> m_detections; // 检测结果列表
    ColumnMetrics m_metrics;                   // 列度量信息
};
}
