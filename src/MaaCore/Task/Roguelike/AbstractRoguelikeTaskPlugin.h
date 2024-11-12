#pragma once

#include <memory>

#include "Task/AbstractTaskPlugin.h"
#include "Task/Roguelike/RoguelikeConfig.h"
#include "Task/Roguelike/RoguelikeControlTaskPlugin.h"

namespace asst
{
class AbstractRoguelikeTaskPlugin : public AbstractTaskPlugin
{
public:
    AbstractRoguelikeTaskPlugin(
        const AsstCallback& callback,
        Assistant* inst,
        std::string_view task_chain,
        const std::shared_ptr<RoguelikeConfig>& data,
        const std::shared_ptr<RoguelikeControlTaskPlugin>& control);

    // 根据 params 设置插件专用参数, 返回插件是否启用
    virtual bool load_params([[maybe_unused]] const json::value& params) { return true; }

    // 进入新的一局重置变量
    virtual void reset_in_run_variables() {}

protected:
    std::shared_ptr<RoguelikeConfig> m_config = nullptr;
    std::shared_ptr<RoguelikeControlTaskPlugin> m_control_ptr = nullptr;
};
} // namespace asst
