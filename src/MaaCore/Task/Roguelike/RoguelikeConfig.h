#pragma once
#include <functional>
#include <meojson/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
    Exp = 0,        // 0 - 刷经验，尽可能稳定地打更多层数，不期而遇采用激进策略
    Investment = 1, // 1 - 刷源石锭，第一层投资完就退出，不期而遇采用保守策略
                    // 2 - 【已移除】两者兼顾，投资过后再退出，没有投资就继续往后打
    Ending = 3,     // 3 - 尝试通关，激进策略（TODO）
    Collectible = 4, // 4 - 刷开局，以获得热水壶或者演讲稿开局或只凹直升，不期而遇采用保守策略

    // ------------------ 萨米主题专用模式 ------------------
    CLP_PDS = 5, // 5 - 刷隐藏坍缩范式,以增加坍缩值为最优先目标

    // ------------------ 萨卡兹主题专用模式 ------------------
    FastPass = 10001 // 10001 - 快速通过第一层
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
        return theme == RoguelikeTheme::Phantom || theme == RoguelikeTheme::Mizuki || theme == RoguelikeTheme::Sami ||
               theme == RoguelikeTheme::Sarkaz;
    }

    static constexpr bool is_valid_mode(RoguelikeMode mode, std::string_view theme = RoguelikeTheme::Sami)
    {
        return mode == RoguelikeMode::Exp || mode == RoguelikeMode::Investment || mode == RoguelikeMode::Collectible ||
               (mode == RoguelikeMode::CLP_PDS && theme == RoguelikeTheme::Sami) ||
               (mode == RoguelikeMode::FastPass && theme == RoguelikeTheme::Sarkaz);
    }

    bool verify_and_load_params(const json::value& params);
    void clear(); // 重置肉鸽局内数据

    // ================================= 通用参数 =================================

public:
    const std::string& get_theme() const { return m_theme; }

    RoguelikeMode get_mode() const { return m_mode; }

    void set_difficulty(int difficulty) { m_difficulty = difficulty; }

    int get_difficulty() const { return m_difficulty; }

    void set_squad(std::string squad) { m_squad = std::move(squad); }

    const std::string& get_squad() const { return m_squad; }

    // ------------------ 开局 ------------------
    void set_start_with_elite_two(bool value) { m_start_with_elite_two = value; }

    bool get_start_with_elite_two() const { return m_start_with_elite_two; }

    void set_only_start_with_elite_two(bool value) { m_only_start_with_elite_two = value; }

    bool get_only_start_with_elite_two() const { return m_only_start_with_elite_two; }

    // ------------------ 投资模式 ------------------
    void set_invest_with_more_score(bool value) { m_invest_with_more_score = value; }

    bool get_invest_with_more_score() const { return m_invest_with_more_score; }

private:
    std::string m_theme;                       // 主题
    RoguelikeMode m_mode = RoguelikeMode::Exp; // 模式
    int m_difficulty = 0;                      // 难度
    std::string m_squad;                       // 分队

    // ------------------ 开局 ------------------
    bool m_start_with_elite_two = false;      // 在刷开局模式下凹开局干员精二直升
    bool m_only_start_with_elite_two = false; // 只凹开局干员精二直升且不进行作战

    // ------------------ 投资模式 ------------------
    bool m_invest_with_more_score = false; // 投资时招募、购物刷分

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
    void set_core_char(std::string core_char) { m_core_char = std::move(core_char); }

    const auto& get_core_char() const { return m_core_char; }

    void set_use_support(bool use_support) { m_use_support = use_support; }

    bool get_use_support() const { return m_use_support; }

    void set_use_nonfriend_support(bool value) { m_use_nonfriend_support = value; }

    bool get_use_nonfriend_support() const { return m_use_nonfriend_support; }

    void set_oper(std::unordered_map<std::string, RoguelikeOper> oper) { m_oper = std::move(oper); }

    const auto& get_oper() const { return m_oper; }

    void set_collection(std::vector<std::string> collection) { m_collection = std::move(collection); }

    const auto& get_collection() const { return m_collection; }

    void set_floor(int floor) { m_floor = floor; }

    int get_floor() const { return m_floor; }

private:
    // ------------------ 招募 ------------------
    bool m_team_full_without_rookie = false; // 编队内没有预干员

    // ------------------ 商店 ------------------
    bool m_trader_no_longer_buy = false; // 不再购买藏品

    // ------------------ 开局 ------------------
    std::string m_core_char;                               // 开局干员名
    bool m_use_support = false;                            // 开局干员是否为助战干员
    bool m_use_nonfriend_support = false;                  // 是否可以是非好友助战干员

    std::unordered_map<std::string, RoguelikeOper> m_oper; // 干员精英&等级
    std::vector<std::string> m_collection;                 // 已获得的藏品
    int m_hope = 0;                                        // 当前希望
    int m_hp = 0;                                          // 当前生命值
    int m_floor = 0;                                       // 当前到达层数
    int m_formation_upper_limit = 6;                       // 当前编队上限

public:
    // ------------------ 密文板 ------------------
    void set_foldartal(auto foldartal) { m_foldartal = std::move(foldartal); }

    const auto& get_foldartal() const { return m_foldartal; }

private:
    int m_chaos = 0; // 抗干扰指
    // ------------------ 密文板 ------------------
    std::vector<std::string> m_foldartal; // 所有已获得密文板

    // ------------------ 萨卡兹主题专用参数 ------------------
public:
    void set_idea_count(int idea_count) { m_idea_count = idea_count; }

    int get_idea_count() const { return m_idea_count; }

    void set_burden_number(int burden_number) { m_burden_number = burden_number; }

    int get_burden_number() const { return m_burden_number; }

    void set_burden_upper_limit(int burden_upper_limit) { m_burden_upper_limit = burden_upper_limit; }

    int get_burden_upper_limit() const { return m_burden_upper_limit; }

private:
    int m_idea_count = 0;         // 构想数量
    int m_burden_number = 0;      // 负荷
    int m_burden_upper_limit = 3; // 负荷上限
};
} // namespace asst
