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
                                    std::shared_ptr<RoguelikeConfig> roguelike_data);

    protected:
        std::shared_ptr<RoguelikeConfig> m_roguelike_config;
    };
}
