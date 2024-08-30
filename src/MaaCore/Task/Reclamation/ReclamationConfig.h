#pragma once
#include <meojson/json.hpp>

namespace asst
{
class ReclamationTheme
{
public:
    static constexpr std::string_view Fire = "Fire";   // Fire Within the Sand
    static constexpr std::string_view Tales = "Tales"; // Tales Within the Sand
};

enum class ReclamationMode // 对应 Roguelike Mode
{
    ProsperityNoSave = 0,  // 0 - 无存档刷繁荣点数
    ProsperityInSave = 1,  // 1 - 有存档刷繁荣点数
};

enum class ReclamationDifficulty // 对应 Roguelike Difficulty
{
    Casual = 0,                  // 0 - 休养模式
    Standard = 1,                // 1 - 标准模式
    Challenge = 2                // 2 - 挑战模式
};

class ReclamationConfig
{
public:
    ReclamationConfig() = default;
    ~ReclamationConfig() = default;

    bool verify_and_load_params(const json::value& params);

    static constexpr bool is_valid_theme(const std::string_view theme) { return theme == ReclamationTheme::Tales; }

    static constexpr bool is_valid_mode(const ReclamationMode& mode, [[maybe_unused]] const std::string_view theme)
    {
        return mode == ReclamationMode::ProsperityNoSave || mode == ReclamationMode::ProsperityInSave;
    }

    static constexpr bool is_valid_difficulty(const ReclamationDifficulty& difficulty)
    {
        return difficulty == ReclamationDifficulty::Casual || difficulty == ReclamationDifficulty::Standard ||
               difficulty == ReclamationDifficulty::Challenge;
    }

    // ———————— 通用参数 ——————————————————————————————————————————————————————————————————
public:
    [[nodiscard]] const std::string& get_theme() const { return m_theme; }

    [[nodiscard]] const ReclamationMode& get_mode() const { return m_mode; }

    [[nodiscard]] ReclamationDifficulty get_difficulty() const { return m_difficulty; }

private:
    std::string m_theme = std::string(ReclamationTheme::Tales);            // 主题
    ReclamationMode m_mode = ReclamationMode::ProsperityInSave;            // 策略
    ReclamationDifficulty m_difficulty = ReclamationDifficulty::Challenge; // 难度模式

    // 以下注释列出了插件专用参数, 以便于快速检阅。这些参数的具体声明与使用请参考各插件。
    // ———————— ReclamationCraftTaskPlugin 专用参数 ———————————————————————————————————————
    // std::string m_tool_to_craft = "荧光棒"; // 要组装的支援道具
    // int m_num_craft_batches = 16;          // 支援道具组装批次数, 每批组装 99 个
};
} // namespace asst
