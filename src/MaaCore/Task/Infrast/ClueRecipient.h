#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace asst::infrast
{
class ClueRecipientRunState
{
public:
    void begin_run(bool retrying) noexcept
    {
        if (!retrying) {
            m_strategy = Strategy::Named;
        }
    }

    void skip() noexcept { m_strategy = Strategy::Skip; }

    void use_default() noexcept
    {
        if (can_send()) {
            m_strategy = Strategy::Default;
        }
    }

    bool can_send() const noexcept { return m_strategy != Strategy::Skip; }

    bool is_using_default() const noexcept { return m_strategy == Strategy::Default; }

private:
    enum class Strategy
    {
        Named,
        Default,
        Skip,
    };
    Strategy m_strategy = Strategy::Named;
};

struct ClueRecipientPage
{
    enum class RowState
    {
        Empty,
        Readable,
        Unreadable,
    };

    std::array<std::string, 4> full_names;
    std::array<std::string, 4> page_keys;
    std::array<RowState, 4> states { RowState::Unreadable,
                                     RowState::Unreadable,
                                     RowState::Unreadable,
                                     RowState::Unreadable };

    bool has_empty_rows() const { return std::ranges::find(states, RowState::Empty) != states.end(); }

    bool can_continue_search() const
    {
        bool empty_tail = false;
        bool has_friend = false;
        for (const auto state : states) {
            if (state == RowState::Unreadable || (empty_tail && state == RowState::Readable)) {
                return false;
            }
            empty_tail |= state == RowState::Empty;
            has_friend |= state == RowState::Readable;
        }
        return has_friend;
    }

    bool operator==(const ClueRecipientPage&) const = default;
};

class ClueRecipientSearchAttempts
{
public:
    enum class Action
    {
        Retry,
        UseDefault,
    };

    Action record_not_found() noexcept
    {
        m_not_found_count = std::min(m_not_found_count + 1, MaxNotFoundCount);
        return m_not_found_count < MaxNotFoundCount ? Action::Retry : Action::UseDefault;
    }

    void reset() noexcept { m_not_found_count = 0; }

    unsigned not_found_count() const noexcept { return m_not_found_count; }

private:
    static constexpr unsigned MaxNotFoundCount = 2;
    unsigned m_not_found_count = 0;
};

inline std::optional<size_t> clue_recipient_send_row(std::string_view task)
{
    constexpr std::string_view prefix = "InfrastClueSendToRecipient";
    if (task.size() != prefix.size() + 1 || !task.starts_with(prefix) || task.back() < '1' || task.back() > '4') {
        return std::nullopt;
    }
    return task.back() - '1';
}

inline bool is_valid_clue_recipient(std::string_view name)
{
    const auto separator = name.rfind('#');
    return separator != std::string_view::npos && separator > 0 && name.size() - separator == 5 &&
           std::ranges::none_of(name, [](unsigned char ch) { return ch < 32 || ch == 127; }) &&
           std::ranges::all_of(name.substr(separator + 1), [](char ch) { return ch >= '0' && ch <= '9'; });
}

inline bool can_exclude_clue_recipient(std::string_view name, std::string_view recipient)
{
    if (name == recipient || !is_valid_clue_recipient(recipient)) {
        return false;
    }
    const auto separator = name.rfind('#');
    if (separator == std::string_view::npos || separator == 0) {
        return false;
    }
    const auto nickname = name.substr(0, separator);
    const auto expected = recipient.substr(0, recipient.rfind('#'));
    // 仅用于排除目标；疑似截断或附带文字仍保留不确定性，不参与正向匹配。
    return (nickname == expected && is_valid_clue_recipient(name)) ||
           (!nickname.starts_with(expected) && !expected.starts_with(nickname));
}

inline std::optional<size_t> find_clue_recipient(std::span<const std::string> names, std::string_view recipient)
{
    if (!is_valid_clue_recipient(recipient)) {
        return std::nullopt;
    }

    std::optional<size_t> row;
    for (size_t index = 0; index < names.size(); ++index) {
        if (names[index] != recipient) {
            continue;
        }
        if (row) {
            return std::nullopt;
        }
        row = index;
    }
    return row;
}
}
