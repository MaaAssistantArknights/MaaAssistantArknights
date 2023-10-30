#pragma once

#include <memory>
#include <string>

#include "RoguelikeData.h"

namespace asst
{
    class RoguelikeInterface
    {
    public:
        void set_roguelike_theme(std::string roguelike_theme) { m_roguelike_theme = std::move(roguelike_theme); }
        void set_roguelike_data(const std::shared_ptr<RoguelikeData> roguelike_data)
        {
            m_roguelike_data = roguelike_data;
        }

    protected:
        // 肉鸽主题
        std::string m_roguelike_theme;
        std::shared_ptr<RoguelikeData> m_roguelike_data;
    };
}
