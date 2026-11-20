#pragma once

namespace cv
{
class Mat;
}

namespace asst
{
struct Rect;

namespace stronghold
{
// badge 是 BGR 截图中完整方牌的外框。无法确认中央字形和字色时返回 0。
int match_tier_badge(const cv::Mat& image, const Rect& badge);
}
}
