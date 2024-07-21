#pragma once
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <meojson/json.hpp>

namespace asst
{
    class RoguelikeTheme
    {
    public:
        static constexpr std::string_view Phantom = "Phantom";
        static constexpr std::string_view Mizuki = "Mizuki";
        static constexpr std::string_view Sami = "Sami";
        static constexpr std::string_view Sarkaz = "Sarkaz";
    };

    enum class RoguelikeMode
    {
        // ------------------ 通用模式 ------------------
        Exp = 0,         // 0 - 刷经验，尽可能稳定地打更多层数，不期而遇采用激进策略
        Investment = 1,  // 1 - 刷源石锭，第一层投资完就退出，不期而遇采用保守策略
                         // 2 - 【已移除】两者兼顾，投资过后再退出，没有投资就继续往后打
        Ending = 3,      // 3 - 尝试通关，激进策略（TODO）
        Collectible = 4, // 4 - 刷开局，以获得热水壶或者演讲稿开局或只凹直升，不期而遇采用保守策略

        // ------------------ 萨米主题专用模式 ------------------
        CLP_PDS = 5      // 5 - 刷隐藏坍缩范式,以增加坍缩值为最优先目标
    };

    struct RoguelikeOper
    {
        int elite = 0; // 精英化
        int level = 0; // 干员等级
    };

    class RoguelikeConfig
    {
    public:
        static constexpr bool is_valid_theme(std::string_view theme)
        {
            return theme == RoguelikeTheme::Phantom ||
                   theme == RoguelikeTheme::Mizuki ||
                   theme == RoguelikeTheme::Sami ||
                   theme == RoguelikeTheme::Sarkaz;
        }
        static constexpr bool is_valid_mode(RoguelikeMode mode, std::string_view theme = RoguelikeTheme::Sami)
        {
            return mode == RoguelikeMode::Exp || 
                   mode == RoguelikeMode::Investment ||
                   mode == RoguelikeMode::Collectible ||
                   (mode == RoguelikeMode::CLP_PDS && theme == RoguelikeTheme::Sami);
        }

        bool set_params(const json::value& params);
        void clear();// 重置肉鸽局内数据

    // ================================= 通用参数 =================================

    public:
        void set_theme(std::string theme) { m_theme = std::move(theme); }
        const std::string& get_theme() const { return m_theme; }
        void set_mode(RoguelikeMode mode) { m_mode = mode; }
        RoguelikeMode get_mode() const { return m_mode; }
        void set_difficulty(int difficulty) { m_difficulty = difficulty; }
        int get_difficulty() const { return m_difficulty; }

        // ------------------ 开局 ------------------
        void set_start_with_elite_two(bool value) { m_start_with_elite_two = value; }
        bool get_start_with_elite_two() const { return m_start_with_elite_two; }
        void set_only_start_with_elite_two(bool value) { m_only_start_with_elite_two = value; }
        bool get_only_start_with_elite_two() const { return m_only_start_with_elite_two; }

        // ------------------ 投资模式 ------------------
        void set_invest_with_more_score(bool value) { m_invest_with_more_score = value; }
        bool get_invest_with_more_score() const { return m_invest_with_more_score; }

    private:
        std::string m_theme;                             // 主题
        RoguelikeMode m_mode = RoguelikeMode::Exp;       // 模式
        int m_difficulty = 0;                            // 难度

        // ------------------ 开局 ------------------
        bool m_start_with_elite_two = false;             // 在刷开局模式下凹开局干员精二直升
        bool m_only_start_with_elite_two = false;        // 只凹开局干员精二直升且不进行作战

        // ------------------ 投资模式 ------------------
        bool m_invest_with_more_score = false;           // 投资时招募、购物刷分
                
    // =========================== 萨米主题专用参数 ===========================

    public:
        // ------------------ 密文板 ------------------
        void set_first_floor_foldartal(bool value) { m_first_floor_foldartal = value; }
        bool get_first_floor_foldartal() const { return m_first_floor_foldartal; }

        // ------------------ 坍缩范式 ------------------
        void set_check_clp_pds(bool value) { m_check_clp_pds = value; }
        bool get_check_clp_pds() const { return m_check_clp_pds; }

    private:
        // ------------------ 密文板 ------------------
        bool m_first_floor_foldartal = false; // 凹远见密文板

        // ------------------ 坍缩范式 ------------------
        bool m_check_clp_pds = false; // 是否检查坍缩范式


    /* 以下为局内数据，每次重置 */
    public:
        // ------------------ 招募 ------------------
        void set_team_full_without_rookie(bool value) { m_team_full_without_rookie = value; }
        bool get_team_full_without_rookie() const { return m_team_full_without_rookie; }

        // ------------------ 商店 ------------------
        void set_trader_no_longer_buy(bool value) { m_trader_no_longer_buy = value; }
        bool get_trader_no_longer_buy() const { return m_trader_no_longer_buy; }

        // ------------------ 开局 ------------------
        void set_squad(std::string squad) { m_squad = std::move(squad); }
        const std::string& get_squad() const { return m_squad; }
        void set_core_char(std::string core_char) { m_core_char = std::move(core_char); }
        const auto& get_core_char() const { return m_core_char; }
        void set_use_support(bool use_support) { m_use_support = use_support; }
        bool get_use_support() const { return m_use_support; }
        void set_use_nonfriend_support(bool value) { m_use_nonfriend_support = value; }
        bool get_use_nonfriend_support() const { return m_use_nonfriend_support; }
        void set_oper(std::unordered_map<std::string, RoguelikeOper> oper) { m_oper = std::move(oper); }
        const auto& get_oper() const { return m_oper; }

    private:
        // ------------------ 招募 ------------------
        bool m_team_full_without_rookie = false;               // 编队内没有预干员

        // ------------------ 商店 ------------------
        bool m_trader_no_longer_buy = false;                   // 不再购买藏品

        // ------------------ 开局 ------------------
        std::string m_squad;                                   // 分队，默认分队为空
        std::string m_core_char;                               // 开局干员名
        bool m_use_support = false;                            // 开局干员是否为助战干员
        bool m_use_nonfriend_support = false;                  // 是否可以是非好友助战干员

        std::unordered_map<std::string, RoguelikeOper> m_oper; // 干员精英&等级


    public:
        // ------------------ 密文板 ------------------
        void set_foldartal(auto foldartal) { m_foldartal = std::move(foldartal); }
        const auto& get_foldartal() const { return m_foldartal; }

        // ------------------ 坍缩范式 ------------------
        void set_clp_pds(std::vector<std::string> clp_pds) { m_clp_pds = std::move(clp_pds); }
        const auto& get_clp_pds() const { return m_clp_pds; }
        void set_need_check_panel(bool need_check_panel) { m_need_check_panel = need_check_panel; }
        bool get_need_check_panel() const { return m_need_check_panel; }

    private:
        // ------------------ 密文板 ------------------
        std::vector<std::string> m_foldartal;         // 所有已获得密文板

        // ------------------ 坍缩范式 ------------------
        std::vector<std::string> m_clp_pds;           // 已受到的坍缩范式
        bool m_need_check_panel = false;              // 是否在下次回到关卡选择界面时检查坍缩范式
    };

    enum class RoguelikeEvent
    {
        None = 0
    };
} // namespace asst
