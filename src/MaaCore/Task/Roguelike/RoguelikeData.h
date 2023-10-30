#pragma once

namespace asst
{
    class RoguelikeData
    {
    public:
        void set_roguelike_theme(std::string roguelike_theme) { m_roguelike_theme = std::move(roguelike_theme); }
        
    private:
        // 肉鸽主题
        std::string m_roguelike_theme;
    };
}
