#pragma once

#include "Task/AbstractTaskPlugin.h"

namespace asst
{
// 基建收获 ｢材料产出｣ 弹窗的回收站产物识别：霍夫圆定位物品图标，格内匹配 template/items
// 材料模板，数量角标 OCR 后发 InfrastRecycleProduct 回调；
// 识别环节不阻断流程，弹窗由任务自身的 ClickRect 点击确认按钮关闭
class InfrastRewardRecycleProductTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~InfrastRewardRecycleProductTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

private:
    virtual bool _run() override;

    static std::vector<cv::Point> detect_icon_centers(const cv::Mat& image);
    static int match_quantity(const cv::Mat& image, const cv::Point& center);

    // 以下尺寸均为 720p 基准实测值：弹窗图标圆环半径约 60，items 模板（约 105px）的圆环半径约 52，
    // 模板按两者之比缩放后与弹窗图标同尺寸
    static constexpr int IconRingRadius = 60;
    static constexpr int TemplRingRadius = 52;
    static constexpr int TemplSourceSize = 105;
    static constexpr int TemplSize = TemplSourceSize * IconRingRadius / TemplRingRadius;
    // 圆环与圆底的渲染和模板不同，只取中心物品本体参与匹配
    static constexpr int BodyRadius = 41;
    // 霍夫圆检出的圆心有几像素误差，格子 roi 四周留余量
    static constexpr int CellPadding = 8;
};
}
