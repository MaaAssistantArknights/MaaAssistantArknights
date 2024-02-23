#pragma once

#include <string>
#include <type_traits>

#include "../common/types.hpp"

namespace json::_jsonization_helper
{
struct next_is_optional_t
{};
struct va_arg_end
{};

struct dumper
{
    template <typename var_t, typename... rest_t>
    json::value _to_json(const char* key, const var_t& var, rest_t&&... rest) const
    {
        json::value result = _to_json(std::forward<rest_t>(rest)...);
        result.emplace(key, var);
        return result;
    }
    template <typename... rest_t>
    json::value _to_json(const char*, next_is_optional_t, rest_t&&... rest) const
    {
        return _to_json(std::forward<rest_t>(rest)...);
    }
    json::value _to_json(va_arg_end) const { return {}; }
};

struct checker
{
    template <typename var_t, typename... rest_t>
    bool _check_json(const json::value& in, std::string& error_key, const char* key, const var_t&,
                     rest_t&&... rest) const
    {
        auto opt = in.find(key);
        if (!opt || !opt->is<var_t>()) {
            error_key = key;
            return false;
        }
        return _check_json(in, error_key, std::forward<rest_t>(rest)...);
    }
    template <typename var_t, typename... rest_t>
    bool _check_json(const json::value& in, std::string& error_key, const char*, next_is_optional_t, const char* key,
                     const var_t&, rest_t&&... rest) const
    {
        auto opt = in.find(key);
        if (opt) {
            if (!opt->is<var_t>()) {
                error_key = key;
                return false;
            }
        } // next_is_optional_t, ignore key not found

        return _check_json(in, error_key, std::forward<rest_t>(rest)...);
    }
    bool _check_json(const json::value&, std::string&, va_arg_end) const { return true; }
};

struct loader
{
    template <typename var_t, typename... rest_t>
    bool _from_json(const json::value& in, std::string& error_key, const char* key, var_t& var, rest_t&&... rest) const
    {
        auto opt = in.find(key);
        if (!opt || !opt->is<var_t>()) {
            error_key = key;
            return false;
        }
        var = std::move(opt)->as<var_t>();

        return _from_json(in, error_key, std::forward<rest_t>(rest)...);
    }
    template <typename var_t, typename... rest_t>
    bool _from_json(const json::value& in, std::string& error_key, const char*, next_is_optional_t, const char* key,
                    var_t& var, rest_t&&... rest) const
    {
        auto opt = in.find(key);
        if (opt) {
            if (!opt->is<var_t>()) {
                error_key = key;
                return false;
            }
            var = std::move(opt)->as<var_t>();
        } // next_is_optional_t, ignore key not found

        return _from_json(in, error_key, std::forward<rest_t>(rest)...);
    }
    bool _from_json(const json::value&, std::string&, va_arg_end) const { return true; }
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

#define _MEOJSON_FOR_EACH_0(pred, ...)
#define _MEOJSON_FOR_EACH_1(pred, x, ...) pred(x)
#define _MEOJSON_FOR_EACH_2(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_1(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_3(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_2(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_4(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_3(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_5(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_4(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_6(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_5(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_7(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_6(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_8(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_7(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_9(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_8(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_10(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_9(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_11(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_10(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_12(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_11(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_13(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_12(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_14(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_13(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_15(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_14(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_16(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_15(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_17(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_16(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_18(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_17(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_19(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_18(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_20(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_19(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_21(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_20(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_22(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_21(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_23(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_22(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_24(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_23(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_25(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_24(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_26(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_25(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_27(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_26(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_28(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_27(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_29(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_28(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_30(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_29(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_31(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_30(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_32(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_31(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_33(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_32(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_34(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_33(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_35(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_34(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_36(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_35(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_37(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_36(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_38(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_37(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_39(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_38(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_40(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_39(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_41(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_40(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_42(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_41(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_43(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_42(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_44(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_43(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_45(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_44(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_46(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_45(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_47(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_46(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_48(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_47(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_49(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_48(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_50(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_49(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_51(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_50(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_52(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_51(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_53(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_52(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_54(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_53(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_55(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_54(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_56(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_55(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_57(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_56(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_58(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_57(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_59(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_58(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_60(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_59(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_61(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_60(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_62(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_61(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_63(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_62(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH_64(pred, x, ...) pred(x) _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_63(pred, __VA_ARGS__))

#define _MEOJSON_ARG_COUNT(...)                                                                                       \
    _MEOJSON_EXPAND(_MEOJSON_ARG_COUNT1(0, ##__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
                                        49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31,   \
                                        30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,   \
                                        11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define _MEOJSON_ARG_COUNT1(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, \
                            _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, \
                            _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, \
                            _56, _57, _58, _59, _60, _61, _62, _63, _64, N, ...)                                      \
    N

#define _MEOJSON_FOR_EACH_(N, pred, ...) _MEOJSON_EXPAND(_MEOJSON_CONCATENATE(_MEOJSON_FOR_EACH_, N)(pred, __VA_ARGS__))
#define _MEOJSON_FOR_EACH(pred, ...) \
    _MEOJSON_EXPAND(_MEOJSON_FOR_EACH_(_MEOJSON_EXPAND(_MEOJSON_ARG_COUNT(__VA_ARGS__)), pred, __VA_ARGS__))

#define _MEOJSON_VARNAME(x) _MEOJSON_CONCATENATE(_meojson_jsonization_, x)
#define _MEOJSON_KEY_VALUE(x) _MEOJSON_STRINGIZE(x), x,
} // namespace json::_private_macro

#define MEO_TOJSON(...)                                                                                    \
    json::value to_json() const                                                                            \
    {                                                                                                      \
        return json::_jsonization_helper::dumper()._to_json(_MEOJSON_EXPAND(                               \
            _MEOJSON_FOR_EACH(_MEOJSON_KEY_VALUE, __VA_ARGS__)) json::_jsonization_helper::va_arg_end {}); \
    }

#define MEO_CHECKJSON(...)                                                                                   \
    bool check_json(const json::value& _MEOJSON_VARNAME(in)) const                                           \
    {                                                                                                        \
        std::string _MEOJSON_VARNAME(error_key);                                                             \
        return check_json(_MEOJSON_VARNAME(in), _MEOJSON_VARNAME(error_key));                                \
    }                                                                                                        \
    bool check_json(const json::value& _MEOJSON_VARNAME(in), std::string& _MEOJSON_VARNAME(error_key)) const \
    {                                                                                                        \
        return json::_jsonization_helper::checker()._check_json(                                             \
            _MEOJSON_VARNAME(in), _MEOJSON_VARNAME(error_key),                                               \
            _MEOJSON_EXPAND(_MEOJSON_FOR_EACH(_MEOJSON_KEY_VALUE, __VA_ARGS__))                              \
                json::_jsonization_helper::va_arg_end {});                                                   \
    }

#define MEO_FROMJSON(...)                                                                             \
    bool from_json(const json::value& _MEOJSON_VARNAME(in))                                           \
    {                                                                                                 \
        std::string _MEOJSON_VARNAME(error_key);                                                      \
        return from_json(_MEOJSON_VARNAME(in), _MEOJSON_VARNAME(error_key));                          \
    }                                                                                                 \
    bool from_json(const json::value& _MEOJSON_VARNAME(in), std::string& _MEOJSON_VARNAME(error_key)) \
    {                                                                                                 \
        return json::_jsonization_helper::loader()._from_json(                                        \
            _MEOJSON_VARNAME(in), _MEOJSON_VARNAME(error_key),                                        \
            _MEOJSON_EXPAND(_MEOJSON_FOR_EACH(_MEOJSON_KEY_VALUE, __VA_ARGS__))                       \
                json::_jsonization_helper::va_arg_end {});                                            \
    }

#define MEO_JSONIZATION(...)                    \
    _MEOJSON_EXPAND(MEO_TOJSON(__VA_ARGS__))    \
    _MEOJSON_EXPAND(MEO_CHECKJSON(__VA_ARGS__)) \
    _MEOJSON_EXPAND(MEO_FROMJSON(__VA_ARGS__))

#define MEO_OPT json::_jsonization_helper::next_is_optional_t {},

#if defined(__clang__)
#pragma clang diagnostic pop // -Wgnu-zero-variadic-macro-arguments
#endif
