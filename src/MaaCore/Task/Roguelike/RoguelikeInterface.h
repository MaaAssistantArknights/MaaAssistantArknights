#pragma once

#include <memory>
#include <string>

#include "RoguelikeData.h"

namespace asst
{
    class RoguelikeInterface
    {
    public:
        void set_roguelike_data(const std::shared_ptr<RoguelikeData> roguelike_data)
        {
            m_roguelike_data = roguelike_data;
        }

    protected:
        std::shared_ptr<RoguelikeData> m_roguelike_data;
    };
}
