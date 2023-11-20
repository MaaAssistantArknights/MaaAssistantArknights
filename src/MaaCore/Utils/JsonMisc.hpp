#pragma once

#include <concepts>
#include <meojson/json.hpp>
#include <vector>

#include "Common/AsstTypes.h"
#include "Logger.hpp"

namespace asst::utils
{
    // base types
    template <typename OutT>
    requires(std::constructible_from<json::value, OutT>)
    bool parse_json_as(const json::value& input, OutT& output)
    {
        if (input.is<OutT>()) {
            output = input.as<OutT>();
            return true;
        }
        return false;
    }

    bool parse_json_as(const json::value& input, AlgorithmType& output)
    {
        if (input.is_string()) {
            output = get_algorithm_type(input.as_string());
            return output != AlgorithmType::Invalid;
        }
        return false;
    }

    bool parse_json_as(const json::value& input, ProcessTaskAction& output)
    {
        if (input.is_string()) {
            output = get_action_type(input.as_string());
            return output != ProcessTaskAction::Invalid;
        }
        return false;
    }

    // std::pair<FirstT, SecondT> <- [first, second]
    template <typename FirstT, typename SecondT>
    requires(requires(const json::value& input, FirstT x, SecondT y) {
        parse_json_as(input, x) && parse_json_as(input, y);
    })
    bool parse_json_as(const json::value& input, std::pair<FirstT, SecondT>& output)
    {
        if (!input.is_array()) {
            return false;
        }
        const auto& items = input.as_array();
        if (items.size() != 2) {
            return false;
        }
        return parse_json_as(items[0], output.first) && parse_json_as(items[1], output.second);
    }

    // std::vector<ValT> <- val | [val, ...]
    template <typename ValT>
    requires(requires(const json::value& input, ValT x) { parse_json_as(input, x); })
    bool parse_json_as(const json::value& input, std::vector<ValT>& output)
    {
        if (!input.is_array()) {
            if constexpr (std::constructible_from<json::value, ValT>) {
                if (input.is<ValT>()) {
                    output = { input.as<ValT>() };
                    return true;
                }
            }
            return false;
        }
        const auto& items = input.as_array();
        output.clear();
        for (const auto& item : items) {
            ValT val;
            if (!parse_json_as(item, val)) {
                output.clear();
                return false;
            }
            output.emplace_back(std::move(val));
        }
        return true;
    }

    // asst::Rect <- [int, int, int, int]
    bool parse_json_as(const json::value& input, asst::Rect& output)
    {
        if (!input.is_array()) {
            return false;
        }
        const auto& items = input.as_array();
        if (items.size() != 4) {
            return false;
        }
        return parse_json_as(items[0], output.x) && parse_json_as(items[1], output.y) &&
               parse_json_as(items[2], output.width) && parse_json_as(items[3], output.height);
    }

    template <typename OutT>
    requires(requires(const json::value& input, OutT& output) { parse_json_as(input, output); })
    std::optional<OutT> parse_json_as(const json::value& input)
    {
        OutT output;
        return parse_json_as(input, output) ? output : std::nullopt;
    }

    template <typename OutT, typename DefaultT>
    requires(std::constructible_from<OutT, DefaultT> || std::constructible_from<OutT, std::invoke_result_t<DefaultT>>)
    bool get_value_or(std::string_view repr, const json::value& input, const std::string& key, OutT& output,
                      DefaultT&& default_val)
    {
        auto opt = input.find(key);
        if (!opt) {
            if constexpr (std::constructible_from<OutT, DefaultT>) {
                output = std::forward<DefaultT>(default_val);
            }
            else {
                output = default_val();
            }
            return true;
        }
        if (parse_json_as(*opt, output)) {
#ifdef ASST_DEBUG
            // 如果有默认值，检查是否与默认值相同
            if constexpr (std::constructible_from<OutT, DefaultT>) {
                if (output == default_val) {
                    Log.warn("Value of", key, "in", repr, "is same as default value");
                }
            }
            else {
                if (output == default_val()) {
                    Log.warn("Value of", key, "in", repr, "is same as default value");
                }
            }
#endif
            return true;
        }
        Log.error("Invalid type of", key, "in", repr);
        return false;
    }

    template <typename OutT, typename DefaultT>
    requires(std::constructible_from<OutT, DefaultT> || std::constructible_from<OutT, std::invoke_result_t<DefaultT>>)
    std::optional<OutT> get_value_or(std::string_view repr, const json::value& input, const std::string& key,
                                     DefaultT&& default_val)
    {
        OutT output;
        return get_value_or(repr, input, key, output, std::forward<DefaultT>(default_val)) ? output : std::nullopt;
    }
} // namespace asst::utils
