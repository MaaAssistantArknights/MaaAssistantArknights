#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace asst
{
    enum class RoguelikeMode
    {
        Exp = 0,        // 0 - 刷经验，尽可能稳定地打更多层数，不期而遇采用激进策略
        Investment = 1, // 1 - 刷源石锭，第一层投资完就退出，不期而遇采用保守策略
                        // 2 - 【已移除】两者兼顾，投资过后再退出，没有投资就继续往后打
                        // 3 - 尝试通关，激进策略（TODO）
        Collectible = 4, // 4 - 刷开局藏品，以获得热水壶或者演讲稿开局，不期而遇采用保守策略
    };

    class RoguelikeTheme
    {
    public:
        static constexpr std::string_view Phantom = "Phantom";
        static constexpr std::string_view Mizuki = "Mizuki";
        static constexpr std::string_view Sami = "Sami";
    };

    struct RoguelikeOper
    {
        int elite = 0; // 精英化
        int level = 0; // 干员等级
    };

    class RoguelikeConfig
    {
    public:
        // 清理缓存的肉鸽数据
        void clear();
        static constexpr bool is_valid_theme(std::string_view theme)
        {
            return theme == RoguelikeTheme::Phantom || theme == RoguelikeTheme::Mizuki || theme == RoguelikeTheme::Sami;
        }
        static constexpr bool is_valid_mode(RoguelikeMode mode)
        {
            return mode == RoguelikeMode::Exp || mode == RoguelikeMode::Investment ||
                   mode == RoguelikeMode::Collectible;
        }

    public:
        void set_theme(std::string theme) { m_theme = std::move(theme); }
        std::string get_theme() { return m_theme; }
        void set_mode(RoguelikeMode mode) { m_mode = mode; }
        RoguelikeMode get_mode() { return m_mode; }
        void set_difficulty(int difficulty) { m_difficulty = difficulty; }
        int get_difficulty() { return m_difficulty; }
        void set_start_with_elite_two(bool start_with_elite_two) { m_start_with_elite_two = start_with_elite_two; }
        bool get_start_with_elite_two() { return m_start_with_elite_two; }

    private:
        std::string m_theme; // 肉鸽主题
        RoguelikeMode m_mode = RoguelikeMode::Exp;
        int m_difficulty = 0;
        bool m_start_with_elite_two = false;

        /* 以下为每次重置 */
    public:
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
        void set_team_full_without_rookie(bool without_rookie) { m_team_full_without_rookie = without_rookie; }
        bool get_team_full_without_rookie() { return m_team_full_without_rookie; }
        void set_use_support(bool use_support) { m_use_support = use_support; }
        bool get_use_support() { return m_use_support; }
        void set_use_nonfriend_support(bool use_nonfriend_support) { m_use_nonfriend_support = use_nonfriend_support; }
        bool get_use_nonfriend_support() { return m_use_nonfriend_support; }
        void set_oper(std::unordered_map<std::string, RoguelikeOper> oper) { m_oper = std::move(oper); }
        const std::unordered_map<std::string, RoguelikeOper>& get_oper() { return m_oper; }
        void set_foldartal_floor(std::optional<std::string> floor) { m_foldartal_floor = std::move(floor); }
        const std::optional<std::string>& get_foldartal_floor() { return m_foldartal_floor; }
        void set_foldartal(std::vector<std::string> foldartal) { m_foldartal = std::move(foldartal); }
        const std::vector<std::string>& get_foldartal() { return m_foldartal; }

    private:
        int m_recruitment_count = 0;                // 肉鸽招募次数
        bool m_recruitment_starts_complete = false; // 开局干员是否已经招募
        bool m_recruitment_team_complete = false;   // 阵容是否完备
        bool m_trader_no_longer_buy = false;
        std::string m_core_char;
        bool m_team_full_without_rookie = false;
        bool m_use_support = false;
        bool m_use_nonfriend_support = false;
        std::unordered_map<std::string, RoguelikeOper> m_oper; // 干员精英&等级
        std::optional<std::string> m_foldartal_floor;          // 当前层的预见密文板，在下一层获得
        std::vector<std::string> m_foldartal;                  // 所有密文板
    };
} // namespace asst
