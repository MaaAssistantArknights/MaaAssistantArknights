#pragma once
#include "FightTimesTaskPlugin.h"
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
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
