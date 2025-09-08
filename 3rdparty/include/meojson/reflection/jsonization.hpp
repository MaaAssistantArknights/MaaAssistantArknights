// IWYU pragma: private, include <meojson/json.hpp>

#pragma once

#include <string>
#include <type_traits>
#include <utility>

#include "../common/types.hpp"
#include "extensions.hpp"

namespace json::_jsonization_helper
{

template <typename value_t>
struct is_optional_t : public std::false_type
{
};

template <typename value_t>
struct is_optional_t<std::optional<value_t>> : public std::true_type
{
};

template <typename value_t>
inline constexpr bool is_optional_v = is_optional_t<value_t>::value;

struct next_is_optional_t
{
};

struct next_override_key_t
{
    const char* key;
};

struct next_state_t
{
    bool is_optional = false;
    const char* override_key = nullptr;
};

struct va_arg_end
{
};

template <typename tag_t>
struct is_tag_t : public std::false_type
{
};

template <>
struct is_tag_t<next_is_optional_t> : public std::true_type
{
};

template <>
struct is_tag_t<next_override_key_t> : public std::true_type
{
};

struct dumper
{
    void _to_json(json::object&, va_arg_end) const {}

    template <typename... rest_t>
    void _to_json(json::object& result, const char* key, rest_t&&... rest) const
    {
        _to_json(result, next_state_t {}, key, std::forward<rest_t>(rest)...);
    }

    template <
        typename var_t,
        typename... rest_t,
        typename _ = std::enable_if_t<!is_tag_t<var_t>::value, void>>
    void _to_json(
        json::object& result,
        next_state_t state,
        const char* key,
        const var_t& var,
        rest_t&&... rest) const
    {
        if (state.override_key) {
            key = state.override_key;
        }
        if constexpr (is_optional_v<var_t>) {
            if (!state.is_optional) {
                throw exception("std::optional must be used with MEO_OPT");
            }

            if (var.has_value()) {
                result.emplace(key, var.value());
            }
        }
        else {
            result.emplace(key, var);
        }
        _to_json(result, std::forward<rest_t>(rest)...);
    }

    template <typename... rest_t>
    void _to_json(
        json::object& result,
        next_state_t state,
        const char*,
        next_is_optional_t,
        rest_t&&... rest) const
    {
        state.is_optional = true;
        _to_json(result, state, std::forward<rest_t>(rest)...);
    }

    template <typename... rest_t>
    void _to_json(
        json::object& result,
        next_state_t state,
        const char*,
        next_override_key_t override_key,
        rest_t&&... rest) const
    {
        state.override_key = override_key.key;
        _to_json(result, state, std::forward<rest_t>(rest)...);
    }
};

struct checker
{
    bool _check_json(const json::value&, std::string&, va_arg_end) const { return true; }

    template <typename... rest_t>
    bool _check_json(
        const json::value& in,
        std::string& error_key,
        const char* key,
        rest_t&&... rest) const
    {
        return _check_json(in, error_key, next_state_t {}, key, std::forward<rest_t>(rest)...);
    }

    template <
        typename var_t,
        typename... rest_t,
        typename _ = std::enable_if_t<!is_tag_t<var_t>::value, void>>
    bool _check_json(
        const json::value& in,
        std::string& error_key,
        next_state_t state,
        const char* key,
        const var_t&,
        rest_t&&... rest) const
    {
        if (state.override_key) {
            key = state.override_key;
        }
        auto opt = in.find(key);
        if constexpr (is_optional_v<var_t>) {
            if (!state.is_optional) {
                throw exception("std::optional must be used with MEO_OPT");
            }

            if (opt && !opt->is<typename var_t::value_type>()) {
                error_key = key;
                return false;
            }
        }
        else {
            if (state.is_optional) {
                if (opt && !opt->is<var_t>()) {
                    error_key = key;
                    return false;
                } // is_optional, ignore key not found
            }
            else if (!opt || !opt->is<var_t>()) {
                error_key = key;
                return false;
            }
        }
        return _check_json(in, error_key, std::forward<rest_t>(rest)...);
    }

    template <typename... rest_t>
    bool _check_json(
        const json::value& in,
        std::string& error_key,
        next_state_t state,
        const char*,
        next_is_optional_t,
        rest_t&&... rest) const
    {
        state.is_optional = true;
        return _check_json(in, error_key, state, std::forward<rest_t>(rest)...);
    }

    template <typename... rest_t>
    bool _check_json(
        const json::value& in,
        std::string& error_key,
        next_state_t state,
        const char*,
        next_override_key_t override_key,
        rest_t&&... rest) const
    {
        state.override_key = override_key.key;
        return _check_json(in, error_key, state, std::forward<rest_t>(rest)...);
    }
};

struct loader
{
    bool _from_json(const json::value&, std::string&, va_arg_end) const { return true; }

    template <typename... rest_t>
    bool
        _from_json(const json::value& in, std::string& error_key, const char* key, rest_t&&... rest)
    {
        return _from_json(in, error_key, next_state_t {}, key, std::forward<rest_t>(rest)...);
    }

    template <
        typename var_t,
        typename... rest_t,
        typename _ = std::enable_if_t<!is_tag_t<var_t>::value, void>>
    bool _from_json(
        const json::value& in,
        std::string& error_key,
        next_state_t state,
        const char* key,
        var_t& var,
        rest_t&&... rest)
    {
        if (state.override_key) {
            key = state.override_key;
        }
        auto opt = in.find(key);
        if constexpr (is_optional_v<var_t>) {
            if (!state.is_optional) {
                throw exception("std::optional must be used with MEO_OPT");
            }

            if (opt && !opt->is<typename var_t::value_type>()) {
                error_key = key;
                return false;
            }
            if (opt) {
                var = std::move(opt)->as<typename var_t::value_type>();
            }
            else {
                var = std::nullopt;
            }
        }
        else {
            if (state.is_optional) {
                if (opt && !opt->is<var_t>()) {
                    error_key = key;
                    return false;
                } // is_optional, ignore key not found
            }
            else if (!opt || !opt->is<var_t>()) {
                error_key = key;
                return false;
            }

            if (opt) {
                var = std::move(opt)->as<var_t>();
            }
        }

        return _from_json(in, error_key, std::forward<rest_t>(rest)...);
    }

    template <typename... rest_t>
    bool _from_json(
        const json::value& in,
        std::string& error_key,
        next_state_t state,
        const char*,
        next_is_optional_t,
        rest_t&&... rest)
    {
        state.is_optional = true;
        return _from_json(in, error_key, state, std::forward<rest_t>(rest)...);
    }

    template <typename... rest_t>
    bool _from_json(
        const json::value& in,
        std::string& error_key,
        next_state_t state,
        const char*,
        next_override_key_t override_key,
        rest_t&&... rest)
    {
        state.override_key = override_key.key;
        return _from_json(in, error_key, state, std::forward<rest_t>(rest)...);
    }
};
} // namespace json::_jsonization_helper

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

namespace json::_private_macro
{
#define _MEOJSON_STRINGIZE(arg) _MEOJSON_STRINGIZE1(arg)
#define _MEOJSON_STRINGIZE1(arg) _MEOJSON_STRINGIZE2(arg)
#define _MEOJSON_STRINGIZE2(arg) #arg

#define _MEOJSON_CONCATENATE(arg1, arg2) _MEOJSON_CONCATENATE1(arg1, arg2)
#define _MEOJSON_CONCATENATE1(arg1, arg2) _MEOJSON_CONCATENATE2(arg1, arg2)
#define _MEOJSON_CONCATENATE2(arg1, arg2) arg1##arg2

#define _MEOJSON_EXPAND(x) x

#define _MEOJSON_FOR_EACH_1(pred, x) pred(x)
#define _MEOJSON_FOR_EACH_2(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_1(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_3(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_2(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_4(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_3(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_5(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_4(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_6(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_5(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_7(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_6(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_8(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_7(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_9(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_8(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_10(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_9(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_11(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_10(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_12(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_11(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_13(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_12(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_14(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_13(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_15(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_14(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_16(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_15(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_17(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_16(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_18(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_17(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_19(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_18(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_20(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_19(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_21(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_20(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_22(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_21(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_23(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_22(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_24(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_23(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_25(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_24(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_26(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_25(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_27(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_26(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_28(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_27(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_29(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_28(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_30(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_29(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_31(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_30(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_32(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_31(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_33(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_32(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_34(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_33(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_35(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_34(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_36(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_35(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_37(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_36(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_38(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_37(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_39(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_38(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_40(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_39(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_41(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_40(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_42(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_41(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_43(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_42(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_44(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_43(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_45(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_44(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_46(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_45(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_47(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_46(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_48(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_47(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_49(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_48(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_50(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_49(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_51(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_50(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_52(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_51(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_53(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_52(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_54(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_53(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_55(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_54(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_56(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_55(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_57(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_56(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_58(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_57(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_59(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_58(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_60(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_59(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_61(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_60(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_62(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_61(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_63(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_62(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_64(pred, x, ...) \
    pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_63(pred, __VA_ARGS__))

#define _MEOJSON_ARG_COUNT(...)          \
    _MEOJSON_EXPAND(_MEOJSON_ARG_COUNT1( \
        0,                               \
        ##__VA_ARGS__,                   \
        64,                              \
        63,                              \
        62,                              \
        61,                              \
        60,                              \
        59,                              \
        58,                              \
        57,                              \
        56,                              \
        55,                              \
        54,                              \
        53,                              \
        52,                              \
        51,                              \
        50,                              \
        49,                              \
        48,                              \
        47,                              \
        46,                              \
        45,                              \
        44,                              \
        43,                              \
        42,                              \
        41,                              \
        40,                              \
        39,                              \
        38,                              \
        37,                              \
        36,                              \
        35,                              \
        34,                              \
        33,                              \
        32,                              \
        31,                              \
        30,                              \
        29,                              \
        28,                              \
        27,                              \
        26,                              \
        25,                              \
        24,                              \
        23,                              \
        22,                              \
        21,                              \
        20,                              \
        19,                              \
        18,                              \
        17,                              \
        16,                              \
        15,                              \
        14,                              \
        13,                              \
        12,                              \
        11,                              \
        10,                              \
        9,                               \
        8,                               \
        7,                               \
        6,                               \
        5,                               \
        4,                               \
        3,                               \
        2,                               \
        1,                               \
        0))
#define _MEOJSON_ARG_COUNT1( \
    _0,                      \
    _1,                      \
    _2,                      \
    _3,                      \
    _4,                      \
    _5,                      \
    _6,                      \
    _7,                      \
    _8,                      \
    _9,                      \
    _10,                     \
    _11,                     \
    _12,                     \
    _13,                     \
    _14,                     \
    _15,                     \
    _16,                     \
    _17,                     \
    _18,                     \
    _19,                     \
    _20,                     \
    _21,                     \
    _22,                     \
    _23,                     \
    _24,                     \
    _25,                     \
    _26,                     \
    _27,                     \
    _28,                     \
    _29,                     \
    _30,                     \
    _31,                     \
    _32,                     \
    _33,                     \
    _34,                     \
    _35,                     \
    _36,                     \
    _37,                     \
    _38,                     \
    _39,                     \
    _40,                     \
    _41,                     \
    _42,                     \
    _43,                     \
    _44,                     \
    _45,                     \
    _46,                     \
    _47,                     \
    _48,                     \
    _49,                     \
    _50,                     \
    _51,                     \
    _52,                     \
    _53,                     \
    _54,                     \
    _55,                     \
    _56,                     \
    _57,                     \
    _58,                     \
    _59,                     \
    _60,                     \
    _61,                     \
    _62,                     \
    _63,                     \
    _64,                     \
    N,                       \
    ...)                     \
    N

#define _MEOJSON_FOR_EACH_(N, pred, ...) \
    _MEOJSON_EXPAND(_MEOJSON_CONCATENATE(_MEOJSON_FOR_EACH_, N)(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH(pred, ...) \
    _MEOJSON_EXPAND(                 \
        _MEOJSON_FOR_EACH_(_MEOJSON_EXPAND(_MEOJSON_ARG_COUNT(__VA_ARGS__)), pred, __VA_ARGS__))

#define _MEOJSON_VARNAME(x) _MEOJSON_CONCATENATE(_meojson_jsonization_, x)
#define _MEOJSON_KEY_VALUE(x) _MEOJSON_STRINGIZE(x), x,
} // namespace json::_private_macro

#define MEO_TOJSON(...)                                                         \
    json::value to_json() const                                                 \
    {                                                                           \
        json::object result;                                                    \
        json::_jsonization_helper::dumper()._to_json(                           \
            result,                                                             \
            _MEOJSON_EXPAND(_MEOJSON_FOR_EACH(_MEOJSON_KEY_VALUE, __VA_ARGS__)) \
                json::_jsonization_helper::va_arg_end {});                      \
        return result;                                                          \
    }

#define MEO_CHECKJSON(...)                                                      \
    bool check_json(const json::value& _MEOJSON_VARNAME(in)) const              \
    {                                                                           \
        std::string _MEOJSON_VARNAME(error_key);                                \
        return check_json(_MEOJSON_VARNAME(in), _MEOJSON_VARNAME(error_key));   \
    }                                                                           \
    bool check_json(                                                            \
        const json::value& _MEOJSON_VARNAME(in),                                \
        std::string& _MEOJSON_VARNAME(error_key)) const                         \
    {                                                                           \
        return json::_jsonization_helper::checker()._check_json(                \
            _MEOJSON_VARNAME(in),                                               \
            _MEOJSON_VARNAME(error_key),                                        \
            _MEOJSON_EXPAND(_MEOJSON_FOR_EACH(_MEOJSON_KEY_VALUE, __VA_ARGS__)) \
                json::_jsonization_helper::va_arg_end {});                      \
    }

#define MEO_FROMJSON(...)                                                       \
    bool from_json(const json::value& _MEOJSON_VARNAME(in))                     \
    {                                                                           \
        std::string _MEOJSON_VARNAME(error_key);                                \
        return from_json(_MEOJSON_VARNAME(in), _MEOJSON_VARNAME(error_key));    \
    }                                                                           \
    bool from_json(                                                             \
        const json::value& _MEOJSON_VARNAME(in),                                \
        std::string& _MEOJSON_VARNAME(error_key))                               \
    {                                                                           \
        return json::_jsonization_helper::loader()._from_json(                  \
            _MEOJSON_VARNAME(in),                                               \
            _MEOJSON_VARNAME(error_key),                                        \
            _MEOJSON_EXPAND(_MEOJSON_FOR_EACH(_MEOJSON_KEY_VALUE, __VA_ARGS__)) \
                json::_jsonization_helper::va_arg_end {});                      \
    }

#define MEO_JSONIZATION(...)                    \
    _MEOJSON_EXPAND(MEO_TOJSON(__VA_ARGS__))    \
    _MEOJSON_EXPAND(MEO_CHECKJSON(__VA_ARGS__)) \
    _MEOJSON_EXPAND(MEO_FROMJSON(__VA_ARGS__))

#define MEO_OPT json::_jsonization_helper::next_is_optional_t {},
#define MEO_KEY(key) json::_jsonization_helper::next_override_key_t { key },

#if defined(__clang__)
#pragma clang diagnostic pop // -Wgnu-zero-variadic-macro-arguments
#endif
