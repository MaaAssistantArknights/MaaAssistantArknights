#pragma once
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
// 博朗台（抠门碎石）任务插件
// 识别理智恢复时间，仅在 5:55 以上才碎石，否则等待
// ref to https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/1607
class DrGrandetTaskPlugin : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~DrGrandetTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    // 识别“理智将在x:xx后恢复”，返回 x:xx 对应的 毫秒 数
    // 若识别失败返回 < 0
    static int analyze_time_left(const cv::Mat& img);

private:
    virtual bool _run() override;
};
}
