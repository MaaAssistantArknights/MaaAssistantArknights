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

        // 根据 params 设置插件专用参数, 返回插件是否适用
        virtual bool set_params([[maybe_unused]] const json::value& params) { return true; }

    protected:
        std::shared_ptr<RoguelikeConfig> m_config;
        std::optional<json::value>
            notify_sibling_plugins(RoguelikeEvent event, const json::value& detail = json::object());
    };

    using RoguelikeTaskPluginPtr = std::shared_ptr<AbstractRoguelikeTaskPlugin>;
}
