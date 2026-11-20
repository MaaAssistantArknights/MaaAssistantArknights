#pragma once

#include <array>
#include <vector>

#include "Common/AsstTypes.h"

namespace asst
{
struct StrongholdStrategyGrid
{
    struct Scrollbar
    {
        std::vector<int> columns;
        std::array<int, 2> track_y;
        std::array<int, 2> scan_padding;
        std::array<int, 2> thumb_width;
        int min_height;
        int min_value;
        int max_saturation;
        double min_fill;
        MEO_JSONIZATION(columns, track_y, scan_padding, thumb_width, min_height, min_value, max_saturation, min_fill);
    };

    struct Frame
    {
        Rect viewport; // x 相对滚动条中心，y 为屏幕坐标。
        int column_step;
        std::array<int, 2> center_from_bottom_left;
        std::array<int, 2> inner_columns; // 左、右边框内侧的采样列，相对卡框左边缘。
        int right_edge;
        std::array<int, 2> height;
        int min_partial_height;
        int candidate_left; // 相对滚动条中心。
        std::array<int, 2> candidate_y;
        MEO_JSONIZATION(
            viewport,
            column_step,
            center_from_bottom_left,
            inner_columns,
            right_edge,
            height,
            min_partial_height,
            candidate_left,
            candidate_y);
    };

    Scrollbar scrollbar;
    Frame frame;
    std::vector<int> columns; // 两种布局的卡心列坐标。
    int portrait_max_saturation;
    int portrait_min_value;
    MEO_FROMJSON(scrollbar, frame, columns, portrait_max_saturation, portrait_min_value);

    bool check_json(const json::value& json) const
    {
        StrongholdStrategyGrid grid {};
        return json.is_object() && grid.from_json(json) && grid.valid();
    }

private:
    bool valid() const;
};
}
