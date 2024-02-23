#pragma once

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <tuple>
#include <variant>

#include "exception.hpp"
#include "utils.hpp"

namespace json
{
template <typename string_t>
class basic_value
{
    using array_ptr = std::unique_ptr<basic_array<string_t>>;
    using object_ptr = std::unique_ptr<basic_object<string_t>>;

public:
    enum class value_type : char
    {
        invalid,
        null,
        boolean,
        string,
        number,
        array,
        object
    };

    using var_t = std::variant<string_t, array_ptr, object_ptr>;
    using char_t = typename string_t::value_type;

public:
    basic_value();
    basic_value(const basic_value<string_t>& rhs);
    basic_value(basic_value<string_t>&& rhs) noexcept;

    basic_value(bool b);

    basic_value(int num);
    basic_value(unsigned num);
    basic_value(long num);
    basic_value(unsigned long num);
    basic_value(long long num);
    basic_value(unsigned long long num);
    basic_value(float num);
    basic_value(double num);
    basic_value(long double num);

    basic_value(const char_t* str);
    basic_value(string_t str);
    basic_value(std::nullptr_t);

    basic_value(basic_array<string_t> arr);
    basic_value(basic_object<string_t> obj);
    basic_value(std::initializer_list<typename basic_object<string_t>::value_type> init_list);

    // Constructed from raw data
    template <typename... args_t>
    basic_value(value_type type, args_t&&... args);

    template <typename collection_t,
              std::enable_if_t<_utils::is_collection<collection_t> &&
                                   std::is_constructible_v<typename basic_array<string_t>::value_type,
                                                           _utils::range_value_t<collection_t>>,
                               bool> = true>
    basic_value(collection_t&& collection) : basic_value(basic_array<string_t>(std::forward<collection_t>(collection)))
    {}
    template <typename map_t, std::enable_if_t<_utils::is_map<map_t> &&
                                                   std::is_constructible_v<typename basic_object<string_t>::value_type,
                                                                           _utils::range_value_t<map_t>>,
                                               bool> = true>
    basic_value(map_t&& map) : basic_value(basic_object<string_t>(std::forward<map_t>(map)))
    {}

    template <typename jsonization_t,
              std::enable_if_t<_utils::has_to_json_in_member<jsonization_t>::value, bool> = true>
    basic_value(const jsonization_t& value) : basic_value(value.to_json())
    {}
    template <typename jsonization_t,
              std::enable_if_t<_utils::has_to_json_in_templ_spec<jsonization_t>::value, bool> = true>
    basic_value(const jsonization_t& value) : basic_value(ext::jsonization<jsonization_t>().to_json(value))
    {}

    template <typename value_t, std::enable_if_t<!std::is_convertible_v<value_t, basic_value<string_t>>, bool> = true>
    basic_value(value_t) = delete;

    // I don't know if you want to convert char to string or number, so I delete these constructors.
    basic_value(char) = delete;
    basic_value(wchar_t) = delete;
    basic_value(char16_t) = delete;
    basic_value(char32_t) = delete;

    ~basic_value();

    bool valid() const noexcept { return _type != value_type::invalid; }
    bool empty() const noexcept { return is_null(); }
    bool is_null() const noexcept { return _type == value_type::null; }
    bool is_number() const noexcept { return _type == value_type::number; }
    bool is_boolean() const noexcept { return _type == value_type::boolean; }
    bool is_string() const noexcept { return _type == value_type::string; }
    bool is_array() const noexcept { return _type == value_type::array; }
    bool is_object() const noexcept { return _type == value_type::object; }
    template <typename value_t>
    bool is() const noexcept;

    template <typename value_t>
    bool all() const;

    bool contains(const string_t& key) const;
    bool contains(size_t pos) const;
    bool exists(const string_t& key) const { return contains(key); }
    bool exists(size_t pos) const { return contains(pos); }
    value_type type() const noexcept { return _type; }
    const basic_value<string_t>& at(size_t pos) const;
    const basic_value<string_t>& at(const string_t& key) const;

    bool erase(size_t pos);
    bool erase(const string_t& key);

    // Usage: get(key_1, key_2, ..., default_value);
    template <typename... key_then_default_value_t>
    auto get(key_then_default_value_t&&... keys_then_default_value) const;

    template <typename value_t = basic_value<string_t>>
    std::optional<value_t> find(size_t pos) const;
    template <typename value_t = basic_value<string_t>>
    std::optional<value_t> find(const string_t& key) const;

    bool as_boolean() const;
    int as_integer() const;
    unsigned as_unsigned() const;
    long as_long() const;
    unsigned long as_unsigned_long() const;
    long long as_long_long() const;
    unsigned long long as_unsigned_long_long() const;
    float as_float() const;
    double as_double() const;
    long double as_long_double() const;
    string_t as_string() const;
    const basic_array<string_t>& as_array() const;
    const basic_object<string_t>& as_object() const;

    template <typename value_t, template <typename...> typename collection_t = std::vector>
    collection_t<value_t> as_collection() const;
    template <typename value_t, template <typename...> typename map_t = std::map>
    map_t<string_t, value_t> as_map() const;

    template <typename value_t>
    value_t as() const;

    basic_array<string_t>& as_array();
    basic_object<string_t>& as_object();

    template <typename... args_t>
    decltype(auto) emplace(args_t&&... args);

    void clear() noexcept;

    string_t dumps(std::optional<size_t> indent = std::nullopt) const { return indent ? format(*indent) : to_string(); }
    // return raw string
    string_t to_string() const;
    string_t format(size_t indent = 4) const { return format(indent, 0); }

    basic_value<string_t>& operator=(const basic_value<string_t>& rhs);
    basic_value<string_t>& operator=(basic_value<string_t>&&) noexcept;
    template <typename value_t, std::enable_if_t<std::is_convertible_v<value_t, basic_value<string_t>>, bool> = true>
    basic_value<string_t>& operator=(const value_t& rhs)
    {
        return *this = basic_value<string_t>(rhs);
    }
    template <typename value_t, std::enable_if_t<std::is_convertible_v<value_t, basic_value<string_t>>, bool> = true>
    basic_value<string_t>& operator=(value_t&& rhs)
    {
        return *this = basic_value<string_t>(std::move(rhs));
    }

    bool operator==(const basic_value<string_t>& rhs) const;
    bool operator!=(const basic_value<string_t>& rhs) const { return !(*this == rhs); }

    const basic_value<string_t>& operator[](size_t pos) const;
    basic_value<string_t>& operator[](size_t pos);
    basic_value<string_t>& operator[](const string_t& key);
    basic_value<string_t>& operator[](string_t&& key);

    basic_value<string_t> operator|(const basic_object<string_t>& rhs) const&;
    basic_value<string_t> operator|(basic_object<string_t>&& rhs) const&;
    basic_value<string_t> operator|(const basic_object<string_t>& rhs) &&;
    basic_value<string_t> operator|(basic_object<string_t>&& rhs) &&;

    basic_value<string_t>& operator|=(const basic_object<string_t>& rhs);
    basic_value<string_t>& operator|=(basic_object<string_t>&& rhs);

    basic_value<string_t> operator+(const basic_array<string_t>& rhs) const&;
    basic_value<string_t> operator+(basic_array<string_t>&& rhs) const&;
    basic_value<string_t> operator+(const basic_array<string_t>& rhs) &&;
    basic_value<string_t> operator+(basic_array<string_t>&& rhs) &&;

    basic_value<string_t>& operator+=(const basic_array<string_t>& rhs);
    basic_value<string_t>& operator+=(basic_array<string_t>&& rhs);

    explicit operator bool() const { return as_boolean(); }
    explicit operator int() const { return as_integer(); }
    explicit operator unsigned() const { return as_unsigned(); }
    explicit operator long() const { return as_long(); }
    explicit operator unsigned long() const { return as_unsigned_long(); }
    explicit operator long long() const { return as_long_long(); }
    explicit operator unsigned long long() const { return as_unsigned_long_long(); }
    explicit operator float() const { return as_float(); }
    explicit operator double() const { return as_double(); }
    explicit operator long double() const { return as_long_double(); }
    explicit operator string_t() const { return as_string(); }

    explicit operator basic_array<string_t>() const { return as_array(); }
    explicit operator basic_object<string_t>() const { return as_object(); }

    template <typename value_t, template <typename...> typename collection_t = std::vector,
              std::enable_if_t<_utils::is_collection<collection_t<value_t>>, bool> = true>
    explicit operator collection_t<value_t>() const
    {
        return as_collection<value_t, collection_t>();
    }
    template <typename value_t, template <typename...> typename map_t = std::map,
              std::enable_if_t<_utils::is_map<map_t<string_t, value_t>>, bool> = true>
    explicit operator map_t<string_t, value_t>() const
    {
        return as_map<value_t, map_t>();
    }
    template <typename jsonization_t,
              std::enable_if_t<_utils::has_from_json_in_member<jsonization_t, string_t>::value, bool> = true>
    explicit operator jsonization_t() const
    {
        jsonization_t dst {};
        if (!dst.from_json(*this)) {
            throw exception("Wrong JSON");
        }
        return dst;
    }
    template <typename jsonization_t,
              std::enable_if_t<_utils::has_from_json_in_templ_spec<jsonization_t, string_t>::value, bool> = true>
    explicit operator jsonization_t() const
    {
        jsonization_t dst {};
        if (!ext::jsonization<jsonization_t>().from_json(*this, dst)) {
            throw exception("Wrong JSON");
        }
        return dst;
    }

private:
    friend class basic_array<string_t>;
    friend class basic_object<string_t>;

    string_t format(size_t indent, size_t indent_times) const;

    static var_t deep_copy(const var_t& src);

    template <typename... key_then_default_value_t, size_t... keys_indexes_t>
    auto get(std::tuple<key_then_default_value_t...> keys_then_default_value,
             std::index_sequence<keys_indexes_t...>) const;

    template <typename value_t, typename first_key_t, typename... rest_keys_t>
    auto get_helper(const value_t& default_value, first_key_t&& first, rest_keys_t&&... rest) const;
    template <typename value_t, typename unique_key_t>
    auto get_helper(const value_t& default_value, unique_key_t&& first) const;

    const string_t& as_basic_type_str() const;
    string_t& as_basic_type_str();

    value_type _type = value_type::null;
    var_t _raw_data;
};

template <typename string_t>
inline basic_value<string_t>::basic_value() = default;

template <typename string_t>
inline basic_value<string_t>::basic_value(const basic_value<string_t>& rhs)
    : _type(rhs._type), _raw_data(deep_copy(rhs._raw_data))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(basic_value<string_t>&& rhs) noexcept = default;

template <typename string_t>
inline basic_value<string_t>::basic_value(bool b)
    : _type(value_type::boolean),
      _raw_data(string_t(b ? _utils::true_string<string_t>() : _utils::false_string<string_t>()))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(int num)
    : _type(value_type::number), _raw_data(_utils::to_basic_string<string_t>(num))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(unsigned num)
    : _type(value_type::number), _raw_data(_utils::to_basic_string<string_t>(num))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(long num)
    : _type(value_type::number), _raw_data(_utils::to_basic_string<string_t>(num))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(unsigned long num)
    : _type(value_type::number), _raw_data(_utils::to_basic_string<string_t>(num))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(long long num)
    : _type(value_type::number), _raw_data(_utils::to_basic_string<string_t>(num))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(unsigned long long num)
    : _type(value_type::number), _raw_data(_utils::to_basic_string<string_t>(num))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(float num)
    : _type(value_type::number), _raw_data(_utils::to_basic_string<string_t>(num))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(double num)
    : _type(value_type::number), _raw_data(_utils::to_basic_string<string_t>(num))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(long double num)
    : _type(value_type::number), _raw_data(_utils::to_basic_string<string_t>(num))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(const char_t* str) : _type(value_type::string), _raw_data(string_t(str))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(string_t str) : _type(value_type::string), _raw_data(std::move(str))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(std::nullptr_t) : _type(value_type::null)
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(basic_array<string_t> arr)
    : _type(value_type::array), _raw_data(std::make_unique<basic_array<string_t>>(std::move(arr)))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(basic_object<string_t> obj)
    : _type(value_type::object), _raw_data(std::make_unique<basic_object<string_t>>(std::move(obj)))
{}

template <typename string_t>
inline basic_value<string_t>::basic_value(std::initializer_list<typename basic_object<string_t>::value_type> init_list)
    : _type(value_type::object), _raw_data(std::make_unique<basic_object<string_t>>(init_list))
{}

// for Pimpl
template <typename string_t>
inline basic_value<string_t>::~basic_value() = default;

template <typename string_t>
template <typename value_t>
inline bool basic_value<string_t>::is() const noexcept
{
    if constexpr (std::is_same_v<basic_value<string_t>, value_t>) {
        return true;
    }
    else if constexpr (_utils::has_check_json_in_member<value_t, string_t>::value) {
        return value_t().check_json(*this);
    }
    else if constexpr (_utils::has_check_json_in_templ_spec<value_t, string_t>::value) {
        return ext::jsonization<value_t>().check_json(*this);
    }
    else if constexpr (std::is_same_v<bool, value_t>) {
        return is_boolean();
    }
    else if constexpr (std::is_arithmetic_v<value_t>) {
        return is_number();
    }
    else if constexpr (std::is_constructible_v<string_t, value_t>) {
        return is_string();
    }
    else if constexpr (std::is_same_v<basic_array<string_t>, value_t>) {
        return is_array();
    }
    else if constexpr (_utils::is_collection<value_t>) {
        return is_array() && all<typename value_t::value_type>();
    }
    else if constexpr (std::is_same_v<basic_object<string_t>, value_t>) {
        return is_object();
    }
    else if constexpr (_utils::is_map<value_t>) {
        return is_object() && std::is_constructible_v<string_t, typename value_t::key_type> &&
               all<typename value_t::mapped_type>();
    }
    else {
        static_assert(!sizeof(value_t), "Unsupported type");
    }
}

template <typename string_t>
inline bool basic_value<string_t>::contains(const string_t& key) const
{
    return is_object() && as_object().contains(key);
}

template <typename string_t>
inline bool basic_value<string_t>::contains(size_t pos) const
{
    return is_array() && as_array().contains(pos);
}

template <typename string_t>
inline const basic_value<string_t>& basic_value<string_t>::at(size_t pos) const
{
    return as_array().at(pos);
}

template <typename string_t>
inline const basic_value<string_t>& basic_value<string_t>::at(const string_t& key) const
{
    return as_object().at(key);
}

template <typename string_t>
inline bool basic_value<string_t>::erase(size_t pos)
{
    return as_array().erase(pos);
}

template <typename string_t>
inline bool basic_value<string_t>::erase(const string_t& key)
{
    return as_object().erase(key);
}

template <typename string_t>
template <typename... key_then_default_value_t>
inline auto basic_value<string_t>::get(key_then_default_value_t&&... keys_then_default_value) const
{
    return get(std::forward_as_tuple(keys_then_default_value...),
               std::make_index_sequence<sizeof...(keys_then_default_value) - 1> {});
}

template <typename string_t>
template <typename... key_then_default_value_t, size_t... keys_indexes_t>
inline auto basic_value<string_t>::get(std::tuple<key_then_default_value_t...> keys_then_default_value,
                                       std::index_sequence<keys_indexes_t...>) const
{
    constexpr unsigned long default_value_index = sizeof...(key_then_default_value_t) - 1;
    return get_helper(std::get<default_value_index>(keys_then_default_value),
                      std::get<keys_indexes_t>(keys_then_default_value)...);
}

template <typename string_t>
template <typename value_t, typename first_key_t, typename... rest_keys_t>
inline auto basic_value<string_t>::get_helper(const value_t& default_value, first_key_t&& first,
                                              rest_keys_t&&... rest) const
{
    if constexpr (std::is_constructible_v<string_t, first_key_t>) {
        return is_object() ? as_object().get_helper(default_value, std::forward<first_key_t>(first),
                                                    std::forward<rest_keys_t>(rest)...)
                           : default_value;
    }
    else if constexpr (std::is_integral_v<std::decay_t<first_key_t>>) {
        return is_array() ? as_array().get_helper(default_value, std::forward<first_key_t>(first),
                                                  std::forward<rest_keys_t>(rest)...)
                          : default_value;
    }
    else {
        static_assert(!sizeof(first_key_t), "Parameter must be integral or string_t constructible");
    }
}

template <typename string_t>
template <typename value_t, typename unique_key_t>
inline auto basic_value<string_t>::get_helper(const value_t& default_value, unique_key_t&& first) const
{
    if constexpr (std::is_constructible_v<string_t, unique_key_t>) {
        return is_object() ? as_object().get_helper(default_value, std::forward<unique_key_t>(first)) : default_value;
    }
    else if constexpr (std::is_integral_v<std::decay_t<unique_key_t>>) {
        return is_array() ? as_array().get_helper(default_value, std::forward<unique_key_t>(first)) : default_value;
    }
    else {
        static_assert(!sizeof(unique_key_t), "Parameter must be integral or string_t constructible");
    }
}

template <typename string_t>
template <typename value_t>
inline std::optional<value_t> basic_value<string_t>::find(size_t pos) const
{
    return is_array() ? as_array().template find<value_t>(pos) : std::nullopt;
}

template <typename string_t>
template <typename value_t>
inline std::optional<value_t> basic_value<string_t>::find(const string_t& key) const
{
    return is_object() ? as_object().template find<value_t>(key) : std::nullopt;
}

template <typename string_t>
inline bool basic_value<string_t>::as_boolean() const
{
    if (is_boolean()) {
        if (const string_t& b_str = as_basic_type_str(); b_str == _utils::true_string<string_t>()) {
            return true;
        }
        else if (b_str == _utils::false_string<string_t>()) {
            return false;
        }
        else {
            throw exception("Unknown Parse Error");
        }
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
inline int basic_value<string_t>::as_integer() const
{
    if (is_number()) {
        return std::stoi(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
inline unsigned basic_value<string_t>::as_unsigned() const
{
    // I don't know why there is no std::stou.
    return static_cast<unsigned>(as_unsigned_long());
}

template <typename string_t>
inline long basic_value<string_t>::as_long() const
{
    if (is_number()) {
        return std::stol(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
inline unsigned long basic_value<string_t>::as_unsigned_long() const
{
    if (is_number()) {
        return std::stoul(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
inline long long basic_value<string_t>::as_long_long() const
{
    if (is_number()) {
        return std::stoll(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
inline unsigned long long basic_value<string_t>::as_unsigned_long_long() const
{
    if (is_number()) {
        return std::stoull(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
inline float basic_value<string_t>::as_float() const
{
    if (is_number()) {
        return std::stof(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
inline double basic_value<string_t>::as_double() const
{
    if (is_number()) {
        return std::stod(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
inline long double basic_value<string_t>::as_long_double() const
{
    if (is_number()) {
        return std::stold(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
inline string_t basic_value<string_t>::as_string() const
{
    if (is_string()) {
        return as_basic_type_str();
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
inline const basic_array<string_t>& basic_value<string_t>::as_array() const
{
    if (is_array()) {
        return *std::get<array_ptr>(_raw_data);
    }

    throw exception("Wrong Type");
}

template <typename string_t>
inline const basic_object<string_t>& basic_value<string_t>::as_object() const
{
    if (is_object()) {
        return *std::get<object_ptr>(_raw_data);
    }

    throw exception("Wrong Type or data empty");
}

template <typename string_t>
inline basic_array<string_t>& basic_value<string_t>::as_array()
{
    if (empty()) {
        *this = basic_array<string_t>();
    }

    if (is_array()) {
        return *std::get<array_ptr>(_raw_data);
    }

    throw exception("Wrong Type");
}

template <typename string_t>
inline basic_object<string_t>& basic_value<string_t>::as_object()
{
    if (empty()) {
        *this = basic_object<string_t>();
    }

    if (is_object()) {
        return *std::get<object_ptr>(_raw_data);
    }

    throw exception("Wrong Type or data empty");
}

template <typename string_t>
template <typename value_t>
inline value_t basic_value<string_t>::as() const
{
    if constexpr (std::is_same_v<basic_value<string_t>, value_t>) {
        return *this;
    }
    else if constexpr (_utils::has_from_json_in_member<value_t, string_t>::value) {
        value_t dst {};
        if (!dst.from_json(*this)) {
            throw exception("Wrong JSON");
        }
        return dst;
    }
    else if constexpr (_utils::has_from_json_in_templ_spec<value_t, string_t>::value) {
        value_t dst {};
        if (!ext::jsonization<value_t>().from_json(*this, dst)) {
            throw exception("Wrong JSON");
        }
        return dst;
    }
    else {
        return static_cast<value_t>(*this);
    }
}

template <typename string_t>
inline const string_t& basic_value<string_t>::as_basic_type_str() const
{
    return std::get<string_t>(_raw_data);
}

template <typename string_t>
inline string_t& basic_value<string_t>::as_basic_type_str()
{
    return std::get<string_t>(_raw_data);
}

template <typename string_t>
template <typename... args_t>
inline decltype(auto) basic_value<string_t>::emplace(args_t&&... args)
{
    constexpr bool is_array_args = std::is_constructible_v<typename basic_array<string_t>::value_type, args_t...>;
    constexpr bool is_object_args = std::is_constructible_v<typename basic_object<string_t>::value_type, args_t...>;

    static_assert(is_array_args || is_object_args, "Args can not constructure a array or object value");

    if constexpr (is_array_args) {
        return as_array().emplace_back(std::forward<args_t>(args)...);
    }
    else if constexpr (is_object_args) {
        return as_object().emplace(std::forward<args_t>(args)...);
    }
}

template <typename string_t>
inline void basic_value<string_t>::clear() noexcept
{
    *this = basic_value<string_t>();
}

template <typename string_t>
inline string_t basic_value<string_t>::to_string() const
{
    switch (_type) {
    case value_type::null:
        return _utils::null_string<string_t>();
    case value_type::boolean:
    case value_type::number:
        return as_basic_type_str();
    case value_type::string:
        return char_t('"') + _utils::unescape_string(as_basic_type_str()) + char_t('"');
    case value_type::array:
        return as_array().to_string();
    case value_type::object:
        return as_object().to_string();
    default:
        throw exception("Unknown basic_value Type");
    }
}

template <typename string_t>
inline string_t basic_value<string_t>::format(size_t indent, size_t indent_times) const
{
    switch (_type) {
    case value_type::null:
    case value_type::boolean:
    case value_type::number:
    case value_type::string:
        return to_string();
    case value_type::array:
        return as_array().format(indent, indent_times);
    case value_type::object:
        return as_object().format(indent, indent_times);
    default:
        throw exception("Unknown basic_value Type");
    }
}

template <typename string_t>
template <typename value_t>
inline bool basic_value<string_t>::all() const
{
    if (is_array()) {
        return as_array().template all<value_t>();
    }
    else if (is_object()) {
        return as_object().template all<value_t>();
    }
    else {
        return false;
    }
}

template <typename string_t>
template <typename value_t, template <typename...> typename collection_t>
inline collection_t<value_t> basic_value<string_t>::as_collection() const
{
    return as_array().template as_collection<value_t, collection_t>();
}

template <typename string_t>
template <typename value_t, template <typename...> typename map_t>
inline map_t<string_t, value_t> basic_value<string_t>::as_map() const
{
    return as_object().template as_map<value_t, map_t>();
}

template <typename string_t>
inline basic_value<string_t>& basic_value<string_t>::operator=(const basic_value<string_t>& rhs)
{
    _type = rhs._type;
    _raw_data = deep_copy(rhs._raw_data);

    return *this;
}

template <typename string_t>
inline basic_value<string_t>& basic_value<string_t>::operator=(basic_value<string_t>&& rhs) noexcept = default;

template <typename string_t>
inline bool basic_value<string_t>::operator==(const basic_value<string_t>& rhs) const
{
    if (_type != rhs._type) return false;

    switch (_type) {
    case value_type::null:
        return rhs.is_null();
    case value_type::boolean:
    case value_type::number:
    case value_type::string:
        return _raw_data == rhs._raw_data;
    case value_type::array:
        return as_array() == rhs.as_array();
    case value_type::object:
        return as_object() == rhs.as_object();
    default:
        throw exception("Unknown basic_value Type");
    }
}

template <typename string_t>
inline const basic_value<string_t>& basic_value<string_t>::operator[](size_t pos) const
{
    // basic_array not support to create by operator[]

    return as_array()[pos];
}

template <typename string_t>
inline basic_value<string_t>& basic_value<string_t>::operator[](size_t pos)
{
    // basic_array not support to create by operator[]

    return as_array()[pos];
}

template <typename string_t>
inline basic_value<string_t>& basic_value<string_t>::operator[](const string_t& key)
{
    if (empty()) {
        *this = basic_object<string_t>();
    }

    return as_object()[key];
}

template <typename string_t>
inline basic_value<string_t>& basic_value<string_t>::operator[](string_t&& key)
{
    if (empty()) {
        *this = basic_object<string_t>();
    }

    return as_object()[std::move(key)];
}

template <typename string_t>
inline basic_value<string_t> basic_value<string_t>::operator|(const basic_object<string_t>& rhs) const&
{
    return as_object() | rhs;
}

template <typename string_t>
inline basic_value<string_t> basic_value<string_t>::operator|(basic_object<string_t>&& rhs) const&
{
    return as_object() | std::move(rhs);
}

template <typename string_t>
inline basic_value<string_t> basic_value<string_t>::operator|(const basic_object<string_t>& rhs) &&
{
    return std::move(as_object()) | rhs;
}

template <typename string_t>
inline basic_value<string_t> basic_value<string_t>::operator|(basic_object<string_t>&& rhs) &&
{
    return std::move(as_object()) | std::move(rhs);
}

template <typename string_t>
inline basic_value<string_t>& basic_value<string_t>::operator|=(const basic_object<string_t>& rhs)
{
    as_object() |= rhs;
    return *this;
}

template <typename string_t>
inline basic_value<string_t>& basic_value<string_t>::operator|=(basic_object<string_t>&& rhs)
{
    as_object() |= std::move(rhs);
    return *this;
}

template <typename string_t>
inline basic_value<string_t> basic_value<string_t>::operator+(const basic_array<string_t>& rhs) const&
{
    return as_array() + rhs;
}

template <typename string_t>
inline basic_value<string_t> basic_value<string_t>::operator+(basic_array<string_t>&& rhs) const&
{
    return as_array() + std::move(rhs);
}

template <typename string_t>
inline basic_value<string_t> basic_value<string_t>::operator+(const basic_array<string_t>& rhs) &&
{
    return std::move(as_array()) + rhs;
}

template <typename string_t>
inline basic_value<string_t> basic_value<string_t>::operator+(basic_array<string_t>&& rhs) &&
{
    return std::move(as_array()) + std::move(rhs);
}

template <typename string_t>
inline basic_value<string_t>& basic_value<string_t>::operator+=(const basic_array<string_t>& rhs)
{
    as_array() += rhs;
    return *this;
}

template <typename string_t>
inline basic_value<string_t>& basic_value<string_t>::operator+=(basic_array<string_t>&& rhs)
{
    as_array() += std::move(rhs);
    return *this;
}

template <typename string_t>
template <typename... args_t>
inline basic_value<string_t>::basic_value(value_type type, args_t&&... args)
    : _type(type), _raw_data(std::forward<args_t>(args)...)
{
    static_assert(std::is_constructible_v<var_t, args_t...>, "Parameter can't be used to construct a var_t");
}

template <typename string_t>
inline typename basic_value<string_t>::var_t basic_value<string_t>::deep_copy(const var_t& src)
{
    var_t dst;
    if (const auto string_ptr = std::get_if<string_t>(&src)) {
        dst = *string_ptr;
    }
    else if (const auto arr_ptr = std::get_if<array_ptr>(&src)) {
        dst = std::make_unique<basic_array<string_t>>(**arr_ptr);
    }
    else if (const auto obj_ptr = std::get_if<object_ptr>(&src)) {
        dst = std::make_unique<basic_object<string_t>>(**obj_ptr);
    }
    else {
        // maybe invalid_value
    }

    return dst;
}

template <typename ostream_t, typename string_t,
          typename std_ostream_t =
              std::basic_ostream<typename string_t::value_type, std::char_traits<typename string_t::value_type>>,
          typename =
              std::enable_if_t<std::is_same_v<std_ostream_t, ostream_t> || std::is_base_of_v<std_ostream_t, ostream_t>>>
ostream_t& operator<<(ostream_t& out, const basic_value<string_t>& val)
{
    out << val.format();
    return out;
}
} // namespace json
