#pragma once
#include <string>

namespace asst
{
    class RoguelikeConfig
    {
    public:
        void set_roguelike_theme(std::string roguelike_theme);

    protected:
        // 肉鸽主题
        std::string m_roguelike_theme;
    };
}
