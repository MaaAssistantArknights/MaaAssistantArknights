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

        // 传递 task->set_params 进行插件配置设置, 返回参数是否合法
        virtual bool set_params([[maybe_unused]] const json::value& params) { return true; }

    protected:
        std::shared_ptr<RoguelikeConfig> m_config;
    };

    using RoguelikeTaskPluginPtr = std::shared_ptr<AbstractRoguelikeTaskPlugin>;
}
