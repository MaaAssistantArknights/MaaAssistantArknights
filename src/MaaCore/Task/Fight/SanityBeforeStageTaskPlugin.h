#pragma once
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
struct SanityResult
{
    int current = 0; // 当前理智
    int max = 0;     // 最大理智
};

class SanityBeforeStageTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~SanityBeforeStageTaskPlugin() override = default;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    // 获取 当前理智/最大理智
    // 返回 是否获取成功
    static std::optional<asst::SanityResult> get_sanity_before_stage(const cv::Mat image);

private:
    virtual bool _run() override;
};
}
