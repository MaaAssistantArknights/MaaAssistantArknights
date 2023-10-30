#pragma once

#include <memory>
#include <string>

#include "RoguelikeData.h"
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
    class AbstractRoguelikeTaskPlugin : public AbstractTaskPlugin
    {
    public:
        AbstractRoguelikeTaskPlugin(const AsstCallback& callback, Assistant* inst, std::string_view task_chain,
                                    std::shared_ptr<RoguelikeData> roguelike_data);

        void set_roguelike_data(const std::shared_ptr<RoguelikeData> roguelike_data)
        {
            m_roguelike_data = roguelike_data;
        }

    protected:
        std::shared_ptr<RoguelikeData> m_roguelike_data;
    };
}
