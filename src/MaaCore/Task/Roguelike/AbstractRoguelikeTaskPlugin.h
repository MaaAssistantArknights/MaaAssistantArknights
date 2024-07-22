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

        virtual void reset_variable() {}

        // 根据 params 设置插件专用参数, 当插件不适用时关闭 m_enable
        virtual void config([[maybe_unused]] const json::value& params) {}

    protected:
        std::shared_ptr<RoguelikeConfig> m_config;
    };

    using RoguelikeTaskPluginPtr = std::shared_ptr<AbstractRoguelikeTaskPlugin>;
}
