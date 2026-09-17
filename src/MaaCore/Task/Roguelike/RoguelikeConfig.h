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
    static constexpr std::string_view JieGarden = "JieGarden";
    static constexpr std::string_view BlackFlow = "BlackFlow";
};

enum class RoguelikeMode
{
    // ------------------ 通用模式 ------------------
    Exp = 0,         // 0 - 刷经验，尽可能稳定地打更多层数，不期而遇采用激进策略
    Investment = 1,  // 1 - 刷源石锭，第一层投资完就退出，不期而遇采用保守策略
                     // 2 - 【已移除】两者兼顾，投资过后再退出，没有投资就继续往后打
    Ending = 3,      // 3 - 尝试通关，激进策略（TODO）
    Collectible = 4, // 4 - 刷开局，以获得热水壶或者演讲稿开局或只凹直升，不期而遇采用保守策略
    Squad = 6,       // 6 - 月度小队，尽可能稳定地打更多层数，不期而遇采用激进策略
    Exploration = 7, // 7 - 深入调查，尽可能稳定地打更多层数，不期而遇采用激进策略

    // ------------------ 萨米主题专用模式 ------------------
    CLP_PDS = 5, // 5 - 刷隐藏坍缩范式,以增加坍缩值为最优先目标

    // ------------------ 萨卡兹主题专用模式 ------------------
    FastPass = 10'001, // 10001 - 快速通过第一层

    // ------------------ 界园主题专用模式 ------------------
    FindPlaytime = 20'001, // 20001 - 刷常乐节点，第一层进洞，找不到需要的节点就重开

    // ------------------ 黑流树海主题专用模式 ------------------
    BlackFlowBabyAnimal = 30'001 // 30001 - 刷襁褓动物
};

struct RoguelikeOper
{
    int elite = 0; // 精英化
    int level = 0; // 干员等级
};

// 开局招募顺位干员，按列表顺序对应开局第 1/2/3 次免费招募
struct RoguelikeStartOper
{
    std::string name;
    bool use_support = false;
};

struct RoguelikeStatus
{
public:
    RoguelikeStatus() = default;
    RoguelikeStatus(const RoguelikeStatus&) = delete;
    RoguelikeStatus& operator=(const RoguelikeStatus&) = default;

public:
    int hope = 0;                                         // 当前希望
    int hp = 0;                                           // 当前生命值
    int floor = 0;                                        // 当前到达层数
    int formation_upper_limit = 6;                        // 当前编队上限

    std::unordered_map<std::string, RoguelikeOper> opers; // 干员精英&等级
    std::vector<std::string> collections;                 // 已获得的藏品

    // ------------------ 招募 ------------------
    bool team_full_without_rookie = false; // 编队内没有预备干员（晋升优先级<200

    // ------------------ 商店 ------------------
    bool trader_no_longer_buy = false; // 不再购买藏品

    // ------------------ 萨米 ------------------
    int chaos = 0;                           // 抗干扰指数
    std::vector<std::string> foldartal_list; // 所有已获得密文板

    // ------------------ 萨卡兹 ------------------
    int idea_count = 0;         // 构想数量
    int burden_number = 0;      // 负荷
    int burden_upper_limit = 3; // 负荷上限

    // ------------------ 界园 ------------------
    int ticket_count = 0; // 票券数量
};

class RoguelikeConfig
{
public:
    static constexpr bool is_valid_theme(std::string_view theme)
    {
        return theme == RoguelikeTheme::Phantom || theme == RoguelikeTheme::Mizuki || theme == RoguelikeTheme::Sami ||
               theme == RoguelikeTheme::Sarkaz || theme == RoguelikeTheme::JieGarden ||
               theme == RoguelikeTheme::BlackFlow;
    }

    static constexpr bool is_valid_mode(RoguelikeMode mode, std::string_view theme = RoguelikeTheme::Sami)
    {
        if (theme == RoguelikeTheme::BlackFlow) {
            return mode == RoguelikeMode::Exp || mode == RoguelikeMode::Investment ||
                   mode == RoguelikeMode::BlackFlowBabyAnimal;
        }
        return mode == RoguelikeMode::Exp || mode == RoguelikeMode::Investment || mode == RoguelikeMode::Collectible ||
               mode == RoguelikeMode::Squad || mode == RoguelikeMode::Exploration ||
               (mode == RoguelikeMode::CLP_PDS && theme == RoguelikeTheme::Sami) ||
               (mode == RoguelikeMode::FastPass && theme == RoguelikeTheme::Sarkaz) ||
               (mode == RoguelikeMode::FindPlaytime && theme == RoguelikeTheme::JieGarden);
    }

    bool verify_and_load_params(const json::value& params);
    void clear(); // 重置肉鸽局内数据

    // 解析开局干员顺位：core_char_list 存在且有非空 name 项时用之（逐项读 name/use_support，name 为空的项跳过）
    static std::vector<RoguelikeStartOper> parse_start_opers(const json::value& params);

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

    void set_run_for_collectible(const bool run_for_collectible) { m_run_for_collectible = run_for_collectible; }

    bool get_run_for_collectible() const { return m_run_for_collectible; }

    void set_skip_recruit_in_fast_pass(const bool value) { m_skip_recruit_in_fast_pass = value; }

    bool get_skip_recruit_in_fast_pass() const { return m_skip_recruit_in_fast_pass; }

    // ------------------ 投资模式 ------------------
    void set_invest_with_more_score(bool value) { m_invest_with_more_score = value; }

    bool get_invest_with_more_score() const { return m_invest_with_more_score; }

    void set_collectible_mode_shopping(bool value) { m_collectible_mode_shopping = value; }

    bool get_collectible_mode_shopping() const { return m_collectible_mode_shopping; }

    // ------------------ 刷常乐节点模式 ------------------
    void set_find_playTime_target(int target) { m_find_playTime_target = target; }

    int get_find_playTime_target() const { return m_find_playTime_target; }

private:
    std::string m_theme;                       // 主题
    RoguelikeMode m_mode = RoguelikeMode::Exp; // 模式
    int m_difficulty = 0;                      // 难度
    std::string m_squad;                       // 分队

    // ------------------ 开局 ------------------
    bool m_start_with_elite_two = false;      // 在刷开局模式下凹开局干员精二直升
    bool m_only_start_with_elite_two = false; // 只凹开局干员精二直升且不进行作战
    bool m_run_for_collectible = false;       // 用于 RoguelikeMode::Collectible，判断是否正在烧水
    bool m_skip_recruit_in_fast_pass = false; // 是否可以在特定情况下跳过招募，是否真的跳过招募需要结合其他条件来判断

    // ------------------ 投资模式 ------------------
    bool m_invest_with_more_score = false; // 投资时招募、购物刷分

    // ------------------ 刷开局模式 ------------------
    bool m_collectible_mode_shopping = false; // 刷开局模式下进入商店时购物

    // ------------------ 刷常乐节点模式 ------------------
    int m_find_playTime_target = 0; // 目标常乐节点子类型 (1=令, 2=黍, 3=年)

private:
    // =========================== 萨米主题专用参数 ===========================
public:
    // ------------------ 密文板 ------------------
    void set_first_floor_foldartal(const bool value) { m_first_floor_foldartal = value; }

    bool get_first_floor_foldartal() const { return m_first_floor_foldartal; }

private:
    // ------------------ 密文板 ------------------
    bool m_first_floor_foldartal = false; // 凹远见密文板

    // ================================================================================
    // 以下为局内数据，每次重置
    // ================================================================================
public:
    auto& status() { return m_status; }

    // ------------------ 开局 ------------------
    void set_start_opers(std::vector<RoguelikeStartOper> opers) { m_start_opers = std::move(opers); }

    const std::vector<RoguelikeStartOper>& get_start_opers() const { return m_start_opers; }

    size_t get_start_oper_index() const { return m_start_oper_index; }

    void reset_start_oper_index() { m_start_oper_index = 0; }

    // 顺位无论招募成败都消耗，下一次开局招募使用下一顺位
    void advance_start_oper_index()
    {
        if (m_start_oper_index < m_start_opers.size()) {
            ++m_start_oper_index;
        }
    }

    const RoguelikeStartOper* get_current_start_oper() const
    {
        return m_start_oper_index < m_start_opers.size() ? &m_start_opers[m_start_oper_index] : nullptr;
    }

    // 兼容旧调用方：第 1 顺位干员名，未指定开局干员时为空串
    std::string get_core_char() const { return m_start_opers.empty() ? std::string {} : m_start_opers.front().name; }

    void set_use_nonfriend_support(bool value) { m_use_nonfriend_support = value; }

    bool get_use_nonfriend_support() const { return m_use_nonfriend_support; }

private:
    RoguelikeStatus m_status; // 局内状态

    // ------------------ 开局 ------------------
    std::vector<RoguelikeStartOper> m_start_opers; // 开局干员顺位列表，任务参数，整局不变
    size_t m_start_oper_index = 0;                 // 下一次开局招募待消耗的顺位，每局重置，跨插件共享
    bool m_use_nonfriend_support = false;          // 是否可以是非好友助战干员
};
} // namespace asst
