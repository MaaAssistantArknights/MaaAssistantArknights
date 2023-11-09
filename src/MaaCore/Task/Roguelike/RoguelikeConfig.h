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
        void set_difficulty(int difficulty) { m_difficulty = difficulty; }
        int get_difficulty() { return m_difficulty; }
        void set_start_with_elite_two(bool start_with_elite_two) { m_start_with_elite_two = start_with_elite_two; }
        bool get_start_with_elite_two() { return m_start_with_elite_two; }

        void set_recruitment_count(int count) { m_recruitment_count = count; }
        int get_recruitment_count() { return m_recruitment_count; }
        void set_recruitment_starts_complete(bool complete) { m_recruitment_starts_complete = complete; }
        bool get_recruitment_starts_complete() { return m_recruitment_starts_complete; }
        void set_recruitment_team_complete(bool complete) { m_recruitment_team_complete = complete; }
        bool get_recruitment_team_complete() { return m_recruitment_team_complete; }
        void set_trader_no_longer_buy(bool no_longer_buy) { m_trader_no_longer_buy = no_longer_buy; }
        bool get_trader_no_longer_buy() { return m_trader_no_longer_buy; }
        void set_core_char(std::string core_char) { m_core_char = std::move(core_char); }
        std::string get_core_char() { return m_core_char; }

    protected:
        // 肉鸽主题
        std::string m_theme;
        RoguelikeMode m_mode = RoguelikeMode::Exp;
        int m_difficulty = 0;
        bool m_start_with_elite_two = false;

        /* 每次重置 */
        // 肉鸽招募次数
        int m_recruitment_count = 0;
        // 开局干员是否已经招募
        bool m_recruitment_starts_complete = false;
        // 阵容是否完备
        bool m_recruitment_team_complete = false;
        bool m_trader_no_longer_buy = false;
        std::string m_core_char;
    };
}
