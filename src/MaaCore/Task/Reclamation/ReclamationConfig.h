#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <meojson/json.hpp>

namespace asst
{
class ReclamationTheme
{
public:
    static constexpr std::string_view Fire = "Fire";                     // Fire Within the Sand
    static constexpr std::string_view Tales = "Tales";                   // Tales Within the Sand
    static constexpr std::string_view RelaunchAnchor = "RelaunchAnchor"; // 重启锚点（RELAUNCH ANCHOR）
};

enum class TalesMode
{
    ProsperityNoSave = 0, // 0 - 无存档刷繁荣点数
    ProsperityInSave = 1, // 1 - 有存档刷繁荣点数
};

enum class RelaunchAnchorMode
{
    RA1 = 1 << 4,  // 1 << 4 - RA-1
    RA15 = 2 << 4, // 2 << 4 - RA-15
    RA4 = 3 << 4,  // 3 << 4 - RA-4
};

using ReclamationMode = std::variant<TalesMode, RelaunchAnchorMode>;

enum class ReclamationDifficulty // 对应 Roguelike Difficulty
{
    Casual = 0,                  // 0 - 休养模式
    Standard = 1,                // 1 - 标准模式
    Challenge = 2                // 2 - 挑战模式
};

class ReclamationConfig
{
public:
    enum class IncrementMode
    {
        Click = 0,
        Hold = 1,
    };

    struct Params
    {
        std::string theme = std::string(ReclamationTheme::Tales);
        ReclamationMode mode = TalesMode::ProsperityInSave;
        bool clear_store = false;
        std::vector<std::string> tools_to_craft;
        int num_craft_batches = 16;
        IncrementMode increment_mode = IncrementMode::Click;
    };

    [[nodiscard]] std::shared_ptr<const Params> get_params() const;

    bool verify_and_load_params(const json::value& params);

    static constexpr bool is_valid_theme(const std::string_view theme)
    {
        return theme == ReclamationTheme::Tales || theme == ReclamationTheme::RelaunchAnchor;
    }

    static constexpr bool is_valid_difficulty(const ReclamationDifficulty& difficulty)
    {
        return difficulty == ReclamationDifficulty::Casual || difficulty == ReclamationDifficulty::Standard ||
               difficulty == ReclamationDifficulty::Challenge;
    }

    [[nodiscard]] ReclamationDifficulty get_difficulty() const { return m_difficulty; }

private:
    mutable std::mutex m_mutex;
    std::shared_ptr<const Params> m_params = std::make_shared<Params>();
    ReclamationDifficulty m_difficulty = ReclamationDifficulty::Challenge;
};
} // namespace asst
