#pragma once

#include "Vision/VisionHelper.h"

#include <string>
#include <vector>

namespace asst
{
struct OcrTaskInfo;

class RoguelikeCoppersAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    ~RoguelikeCoppersAnalyzer() override = default;

    struct ColumnMetrics
    {
        int origin_x = 0;
        int origin_y = 0;
        int row_offset = 0;
    };

    enum class ColumnRole
    {
        Leftmost,
        Middle,
        Rightmost
    };

    struct CopperDetection
    {
        Rect match_rect;
        Rect name_roi;
        Rect cast_roi;
        Point click_point;
        std::string templ_name;
        std::string name;
        double name_score = 0.0;
        double cast_score = 0.0;
        bool cast_recognized = false;
        bool is_cast = false;
    };

    bool analyze_pickup();
    bool analyze_column(ColumnRole role, bool detect_cast = true);

    const std::vector<CopperDetection>& get_detections() const noexcept { return m_detections; }

    const ColumnMetrics& get_column_metrics() const noexcept { return m_metrics; }

private:
    enum class SortStrategy
    {
        Horizontal,
        Vertical
    };

    bool analyze_internal(
        const std::string& matcher_task,
        const OcrTaskInfo& name_task,
        const OcrTaskInfo* cast_task,
        SortStrategy sort_strategy);

    std::vector<CopperDetection> m_detections;
    ColumnMetrics m_metrics;
};
}
