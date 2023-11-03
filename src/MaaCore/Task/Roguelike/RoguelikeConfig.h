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

    class RoguelikeConfig
    {
    public:
        // 清理缓存的肉鸽数据
        void clear();

    public:
        void set_theme(std::string theme) { m_theme = std::move(theme); }
        std::string get_theme() { return m_theme; }
        void set_mode(RoguelikeMode mode) { m_mode = mode; }
        RoguelikeMode get_mode() { return m_mode; }

    protected:
        // 肉鸽主题
        std::string m_theme;
        RoguelikeMode m_mode = RoguelikeMode::Exp;
    };
}
