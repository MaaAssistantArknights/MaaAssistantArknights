#pragma once
#include <string>

namespace asst
{
    class RoguelikeConfig
    {
    public:
        void clear();

    public:
        void set_roguelike_theme(std::string roguelike_theme) { m_roguelike_theme = std::move(roguelike_theme); }
        std::string get_theme() { return m_roguelike_theme; }

    protected:
        // 肉鸽主题
        std::string m_roguelike_theme;
    };
}
