#pragma once

#include <string>

namespace asst
{
    enum class RoguelikeMode
    {
        // 0 - 刷经验，尽可能稳定地打更多层数，不期而遇采用激进策略
        Exp = 0,
        // 1 - 刷源石锭，第一层投资完就退出，不期而遇采用保守策略
        Investment = 1,
        // 2 - 【已移除】两者兼顾，投资过后再退出，没有投资就继续往后打
        // 3 - 尝试通关，激进策略（TODO）
        
        // 4 - 刷开局藏品，以获得热水壶或者演讲稿开局，不期而遇采用保守策略
        Collectible = 4,
    };
    class RoguelikeData
    {
    public:
        void set_theme(std::string roguelike_theme) { m_roguelike_theme = std::move(roguelike_theme); }
        std::string get_theme() { return m_roguelike_theme; }
        void set_roguelike_mode(RoguelikeMode roguelike_mode) { m_roguelike_mode = roguelike_mode; }
        RoguelikeMode get_roguelike_mode() { return m_roguelike_mode; }

        void clear();

    private:
        // 肉鸽主题
        std::string m_roguelike_theme;
        RoguelikeMode m_roguelike_mode;
    };
}
