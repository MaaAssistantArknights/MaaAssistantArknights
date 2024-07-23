#pragma once

#include <memory>

#include "Task/AbstractTaskPlugin.h"
#include "Task/Roguelike/RoguelikeConfig.h"

namespace asst
{
    class AbstractRoguelikeTaskPlugin : public AbstractTaskPlugin
    {
    public:
        AbstractRoguelikeTaskPlugin(const AsstCallback& callback, Assistant* inst, std::string_view task_chain,
                                    const std::shared_ptr<RoguelikeConfig>& data);

        // 根据 params 设置插件专用参数, 返回插件是否适用
        virtual bool set_params([[maybe_unused]] const json::value& params) { return true; }

        // 进入新的一轮坍缩时重置局内变量
        virtual void reset_in_run_variables() {}

    protected:
        std::shared_ptr<RoguelikeConfig> m_config;
    };

    using RoguelikeTaskPluginPtr = std::shared_ptr<AbstractRoguelikeTaskPlugin>;
}
