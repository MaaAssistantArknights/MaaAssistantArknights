#pragma once
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
// 制造站“源石碎片”自动补货任务插件
class ReplenishOriginiumShardTaskPlugin : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~ReplenishOriginiumShardTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    /// 本轮任务运行内补货流程是否执行过：补货可能留下未执行的制造计划
    /// （确认按钮未弹或未被点中时），宿主任务据此决定切页签时是否探测
    /// ｢尚未执行新的制造计划｣ 弹窗，其余场景无该弹窗，不必白跑 OCR
    bool ran_this_run() const noexcept { return m_ran; }

private:
    virtual bool _run() override;

    bool m_ran = false;
};
}
