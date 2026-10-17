#pragma once

#include <algorithm>
#include <charconv>
#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace asst::activity_navigation
{
using Time = std::chrono::sys_seconds;

inline std::optional<Time> parse_time(std::string_view text)
{
    if ((text.size() != 25 && text.size() != 20) || text[4] != '-' || text[7] != '-' || text[10] != 'T' ||
        text[13] != ':' || text[16] != ':') {
        return std::nullopt;
    }
    const auto number = [&](size_t pos, size_t count) -> int {
        int result = -1;
        auto [end, error] = std::from_chars(text.data() + pos, text.data() + pos + count, result);
        return error == std::errc() && end == text.data() + pos + count ? result : -1;
    };
    const int year = number(0, 4), month = number(5, 2), day = number(8, 2);
    const int hour = number(11, 2), minute = number(14, 2), second = number(17, 2);
    const std::chrono::year_month_day date { std::chrono::year(year),
                                             std::chrono::month(month),
                                             std::chrono::day(day) };
    if (!date.ok() || year < 2000 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return std::nullopt;
    }
    int offset = 0;
    if (text.size() == 20) {
        if (text[19] != 'Z') {
            return std::nullopt;
        }
    }
    else {
        if ((text[19] != '+' && text[19] != '-') || text[22] != ':') {
            return std::nullopt;
        }
        const int hours = number(20, 2), minutes = number(23, 2);
        if (hours < 0 || hours > 14 || minutes < 0 || minutes > 59 || (hours == 14 && minutes != 0)) {
            return std::nullopt;
        }
        offset = (hours * 60 + minutes) * (text[19] == '+' ? 1 : -1);
    }
    return std::chrono::sys_days(date) + std::chrono::hours(hour) + std::chrono::minutes(minute - offset) +
           std::chrono::seconds(second);
}

struct Route
{
    std::string stage_id;
    std::string code;
    std::string activity;
    std::string partition;
    std::string task;
    std::vector<std::string> clients;
    bool enabled = false;
    bool normal = false;
    bool raid = false;
    Time opens_at;
    Time closes_at;
};

enum class Status
{
    Ready,
    Disabled,
    WrongClient,
    WrongCode,
    UnsupportedMode,
    Closed
};

inline Status check(const Route& route, std::string_view code, bool raid, std::string_view client, Time now)
{
    if (!route.enabled) {
        return Status::Disabled;
    }
    if (std::find(route.clients.begin(), route.clients.end(), client) == route.clients.end()) {
        return Status::WrongClient;
    }
    if (route.code != code) {
        return Status::WrongCode;
    }
    if (raid ? !route.raid : !route.normal) {
        return Status::UnsupportedMode;
    }
    if (now < route.opens_at || now >= route.closes_at) {
        return Status::Closed;
    }
    return Status::Ready;
}
}
