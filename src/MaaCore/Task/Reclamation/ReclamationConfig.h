#pragma once
#include <meojson/json.hpp>

namespace asst
{
class ReclamationTheme
{
public:
    static constexpr std::string_view Fire = "Fire";   // Fire Within The Sand
    static constexpr std::string_view Tales = "Tales"; // Tales Within The Sand
};

enum class ReclamationStrategy // 对应 Roguelike Mode
{
    ProsperityNoSave = 0,      // 0 - 无存档刷繁荣点数
    ProsperityInSave = 1,      // 1 - 有存档刷繁荣点数
};

enum class ReclamationMode // 对应 Roguelike Difficulty
{
    Casual = 0,            // 0 - 休养模式
    Standard = 1,          // 1 - 标准模式
    Challenge = 2          // 2 - 挑战模式
};

class ReclamationConfig
{
public:
    ReclamationConfig() = default;
    ~ReclamationConfig() = default;

    bool verify_and_load_params(const json::value& params);

    static constexpr bool is_valid_theme(std::string_view theme)
    {
        return theme == ReclamationTheme::Fire || theme == ReclamationTheme::Tales;
    }

    static constexpr bool
        is_valid_strategy(const ReclamationStrategy& strategy, [[maybe_unused]] const std::string_view& theme)
    {
        return strategy == ReclamationStrategy::ProsperityNoSave || strategy == ReclamationStrategy::ProsperityInSave;
    }

    static constexpr bool
        is_valid_mode(const ReclamationMode& mode, [[maybe_unused]] const ReclamationStrategy& strategy)
    {
        return mode == ReclamationMode::Casual || mode == ReclamationMode::Standard ||
               mode == ReclamationMode::Challenge;
    }

    // ———————— 通用参数 ——————————————————————————————————————————————————————————————————
public:
    void set_theme(const std::string& theme);

    [[nodiscard]] const std::string& get_theme() const { return m_theme; }

    void set_strategy(const ReclamationStrategy& strategy);

    [[nodiscard]] const ReclamationStrategy& get_strategy() const { return m_strategy; }

    void set_mode(const ReclamationMode& mode);

    [[nodiscard]] ReclamationMode get_mode() const { return m_mode; }

private:
    std::string m_theme = std::string(ReclamationTheme::Tales);             // 主题
    ReclamationStrategy m_strategy = ReclamationStrategy::ProsperityInSave; // 策略
    ReclamationMode m_mode = ReclamationMode::Challenge;                    // 模式

    // ———————— ReclamationStrategy::ProsperityInSave 专用参数 ————————————————————————————
public:
    void set_tool_to_craft(const std::string& tool) { m_tool_to_craft = tool; };
    [[nodiscard]] std::string get_tool_to_craft() const { return m_tool_to_craft; }
private:
    std::string m_tool_to_craft; // 要组装的支援道具

};
} // namespace asst
