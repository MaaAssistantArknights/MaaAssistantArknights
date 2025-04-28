// IWYU pragma: private, include <meojson/json.hpp>

#pragma once

#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace json
{
template <typename string_t>
class basic_value;
template <typename string_t>
class basic_array;
template <typename string_t>
class basic_object;

using default_string_t = std::string;

using value = basic_value<default_string_t>;
using array = basic_array<default_string_t>;
using object = basic_object<default_string_t>;

using wvalue = basic_value<std::wstring>;
using warray = basic_array<std::wstring>;
using wobject = basic_object<std::wstring>;
}

namespace json::ext
{
template <typename T>
class jsonization
{
public:
    // json::value to_json(const T&) const;
    // bool check_json(const json::value&) const;
    // bool from_json(const json::value&, T&) const;
};
}

namespace json::_utils
{
template <typename T>
using iterator_t = decltype(std::declval<T&>().begin());
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
template <typename T>
using iter_value_t = typename std::iterator_traits<remove_cvref_t<T>>::value_type;
template <typename R>
using range_value_t = iter_value_t<iterator_t<R>>;

template <typename T, typename = void>
constexpr bool is_string = false;
template <typename T>
constexpr bool is_string<T, std::void_t<typename T::traits_type>> =
    std::is_same_v<typename T::traits_type, std::char_traits<typename T::value_type>>;

template <typename T, typename = void>
constexpr bool is_container = false;
template <typename T>
constexpr bool is_container<T, std::void_t<typename T::value_type, range_value_t<T>>> =
    std::is_same_v<typename T::value_type, range_value_t<T>> && !is_string<T>;

template <typename T, typename = void>
constexpr bool is_map = false;
template <typename T>
constexpr bool is_map<T, std::void_t<typename T::key_type, typename T::mapped_type>> =
    is_container<T>;

template <typename T, typename = void>
constexpr bool is_fixed_array = false;
template <template <typename, size_t> typename arr_t, typename value_t, size_t size>
constexpr bool is_fixed_array<arr_t<value_t, size>> = true;

template <typename T, typename = std::enable_if_t<is_fixed_array<T>>>
constexpr size_t fixed_array_size = 0;
template <template <typename, size_t> typename arr_t, typename value_t, size_t size>
constexpr size_t fixed_array_size<arr_t<value_t, size>> = size;

template <typename T, typename = void>
constexpr bool is_collection = false;
template <typename T>
constexpr bool is_collection<T> = is_container<T> && !is_map<T> && !is_fixed_array<T>;

template <typename T>
constexpr bool is_variant = false;
template <typename... args_t>
constexpr bool is_variant<std::variant<args_t...>> = true;

template <typename T>
constexpr bool is_tuple = false;
template <typename... args_t>
constexpr bool is_tuple<std::tuple<args_t...>> = true;

template <typename T>
constexpr bool is_pair = false;
template <typename... args_t>
constexpr bool is_pair<std::pair<args_t...>> = true;

template <typename T>
class has_to_json_in_member
{
    template <typename U>
    static auto test(int) -> decltype(std::declval<U>().to_json(), std::true_type());

    template <typename U>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template <typename T>
class has_to_json_in_templ_spec
{
    template <typename U>
    static auto test(int)
        -> decltype(std::declval<ext::jsonization<U>>().to_json(std::declval<U>()), std::true_type());

    template <typename U>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template <typename T, typename string_t>
class has_check_json_in_member
{
    template <typename U>
    static auto test(int)
        -> decltype(std::declval<U>().check_json(std::declval<json::basic_value<string_t>>()), std::true_type());

    template <typename U>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template <typename T, typename string_t>
class has_check_json_in_templ_spec
{
    template <typename U>
    static auto test(int)
        -> decltype(std::declval<ext::jsonization<U>>().check_json(std::declval<json::basic_value<string_t>>()), std::true_type());

    template <typename U>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template <typename T, typename string_t>
class has_from_json_in_member
{
    template <typename U>
    static auto test(int)
        -> decltype(std::declval<U>().from_json(std::declval<json::basic_value<string_t>>()), std::true_type());

    template <typename U>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template <typename T, typename string_t>
class has_from_json_in_templ_spec
{
    template <typename U>
    static auto test(int)
        -> decltype(std::declval<ext::jsonization<U>>().from_json(std::declval<json::basic_value<string_t>>(), std::declval<U&>()), std::true_type());

    template <typename U>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template <typename string_t>
inline constexpr string_t unescape_string(const string_t& str)
{
    using char_t = typename string_t::value_type;

    string_t result {};
    auto cur = str.cbegin();
    auto end = str.cend();
    auto no_escape_beg = cur;
    char_t escape = 0;

    for (; cur != end; ++cur) {
        switch (*cur) {
        case '"':
            escape = '"';
            break;
        case '\\':
            escape = '\\';
            break;
        case '\b':
            escape = 'b';
            break;
        case '\f':
            escape = 'f';
            break;
        case '\n':
            escape = 'n';
            break;
        case '\r':
            escape = 'r';
            break;
        case '\t':
            escape = 't';
            break;
        default:
            break;
        }
        if (escape) {
            result += string_t(no_escape_beg, cur) + char_t('\\') + escape;
            no_escape_beg = cur + 1;
            escape = 0;
        }
    }
    result += string_t(no_escape_beg, cur);

    return result;
}

template <typename string_t>
inline constexpr string_t true_string()
{
    return { 't', 'r', 'u', 'e' };
}

template <typename string_t>
inline constexpr string_t false_string()
{
    return { 'f', 'a', 'l', 's', 'e' };
}

template <typename string_t>
inline constexpr string_t null_string()
{
    return { 'n', 'u', 'l', 'l' };
}

template <typename string_t, typename any_t>
inline string_t to_basic_string(any_t&& arg)
{
#ifdef MEOJSON_KEEP_FLOATING_PRECISION
    using real_type = std::remove_reference_t<any_t>;
    if constexpr (std::is_floating_point_v<real_type>) {
        if constexpr (std::is_same_v<string_t, std::string>) {
            std::ostringstream oss;
            oss << std::setprecision(std::numeric_limits<real_type>::max_digits10) << arg;
            return oss.str();
        }
        else if constexpr (std::is_same_v<string_t, std::wstring>) {
            std::wostringstream oss;
            oss << std::setprecision(std::numeric_limits<real_type>::max_digits10) << arg;
            return oss.str();
        }
    }
#endif
    if constexpr (std::is_same_v<string_t, std::string>) {
        return std::to_string(std::forward<any_t>(arg));
    }
    else if constexpr (std::is_same_v<string_t, std::wstring>) {
        return std::to_wstring(std::forward<any_t>(arg));
    }
    else {
        static_assert(!sizeof(any_t), "Unsupported type");
    }
}

template <std::size_t id, typename string_t, typename variant_t>
inline bool _serialize_variant_impl(basic_value<string_t>& val, variant_t&& var)
{
    if (var.index() != id) {
        return false;
    }
    val = basic_value<string_t>(std::get<id>(std::forward<variant_t>(var)));
    return true;
}

template <typename string_t, typename variant_t, std::size_t... ids>
inline basic_value<string_t> serialize_variant(variant_t&& var, std::index_sequence<ids...>)
{
    basic_value<string_t> val;
    (_serialize_variant_impl<ids>(val, std::forward<variant_t>(var)) || ...);
    return val;
}

template <std::size_t id, typename string_t, typename variant_t>
inline bool _deserialize_variant_impl(const basic_value<string_t>& val, variant_t& var)
{
    using alt_t = std::variant_alternative_t<id, variant_t>;
    if (!val.template is<alt_t>()) {
        return false;
    }
    var = val.template as<alt_t>();
    return true;
}

template <typename string_t, typename variant_t, std::size_t... ids>
inline variant_t deserialize_variant(const basic_value<string_t>& val, std::index_sequence<ids...>)
{
    variant_t var;
    (_deserialize_variant_impl<ids>(val, var) || ...);
    return var;
}

template <typename string_t, typename variant_t, std::size_t... ids>
inline bool detect_variant(const basic_value<string_t>& val, std::index_sequence<ids...>)
{
    return (val.template is<std::variant_alternative_t<ids, variant_t>>() || ...);
}

template <typename string_t, typename tuple_t, std::size_t... ids>
inline bool detect_tuple(const basic_value<string_t>& val, std::index_sequence<ids...>)
{
    return val.is_array() && val.as_array().size() == std::tuple_size_v<tuple_t>
           && (val.at(ids).template is<std::tuple_element_t<ids, tuple_t>>() || ...);
}
} // namespace json::_utils
