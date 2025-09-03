// IWYU pragma: private, include <meojson/json.hpp>

#pragma once

#include <initializer_list>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <tuple>

#include "exception.hpp"
#include "utils.hpp"

namespace json
{
template <typename string_t>
class basic_object
{
    friend class basic_value<string_t>;
    friend class basic_array<string_t>;

public:
    using raw_object = std::map<string_t, basic_value<string_t>>;
    using key_type = typename raw_object::key_type;
    using mapped_type = typename raw_object::mapped_type;
    using value_type = typename raw_object::value_type;
    using iterator = typename raw_object::iterator;
    using const_iterator = typename raw_object::const_iterator;
    using char_t = typename string_t::value_type;

public:
    basic_object() = default;
    basic_object(const basic_object<string_t>& rhs) = default;
    basic_object(basic_object<string_t>&& rhs) noexcept = default;
    basic_object(std::initializer_list<value_type> init_list);

    // explicit basic_object(const basic_value<string_t>& val);
    // explicit basic_object(basic_value<string_t>&& val);

    template <
        typename jsonization_t,
        std::enable_if_t<
            _utils::has_to_json_in_templ_spec<jsonization_t, string_t>::value
                && !_utils::has_to_json_object_in_templ_spec<jsonization_t, string_t>::value,
            bool> = true>
    basic_object(const jsonization_t& value)
        : basic_object(ext::jsonization<string_t, jsonization_t>().to_json(value))
    {
    }

    template <
        typename jsonization_t,
        std::enable_if_t<
            _utils::has_to_json_object_in_templ_spec<jsonization_t, string_t>::value,
            bool> = true>
    basic_object(const jsonization_t& value)
        : basic_object(ext::jsonization<string_t, jsonization_t>().to_json_object(value))
    {
    }

    template <
        typename jsonization_t,
        std::enable_if_t<
            std::is_rvalue_reference_v<jsonization_t&&>
                && _utils::has_move_to_json_in_templ_spec<jsonization_t, string_t>::value
                && !_utils::has_move_to_json_object_in_templ_spec<jsonization_t, string_t>::value,
            bool> = true>
    basic_object(jsonization_t&& value)
        : basic_object(ext::jsonization<string_t, jsonization_t>().move_to_json(std::move(value)))
    {
    }

    template <
        typename jsonization_t,
        std::enable_if_t<
            std::is_rvalue_reference_v<jsonization_t&&>
                && _utils::has_move_to_json_object_in_templ_spec<jsonization_t, string_t>::value,
            bool> = true>
    basic_object(jsonization_t&& value)
        : basic_object(
              ext::jsonization<string_t, jsonization_t>().move_to_json_object(std::move(value)))
    {
    }

    ~basic_object() = default;

    bool empty() const noexcept { return _object_data.empty(); }

    size_t size() const noexcept { return _object_data.size(); }

    bool contains(const string_t& key) const;

    bool exists(const string_t& key) const { return contains(key); }

    const basic_value<string_t>& at(const string_t& key) const;

    string_t dumps(std::optional<size_t> indent = std::nullopt) const
    {
        return indent ? format(*indent) : to_string();
    }

    string_t to_string() const;

    string_t format(size_t indent = 4) const { return format(indent, 0); }

    template <typename value_t>
    bool all() const;
    template <typename value_t, template <typename...> typename map_t = std::map>
    map_t<string_t, value_t> as_map() const;

    template <
        typename value_t,
        std::enable_if_t<
            _utils::has_from_json_object_in_templ_spec<value_t, string_t>::value,
            bool> = true>
    value_t as() const&
    {
        value_t res;
        ext::jsonization<string_t, value_t>().from_json_object(*this, res);
        return res;
    }

    template <
        typename value_t,
        std::enable_if_t<
            _utils::has_move_from_json_object_in_templ_spec<value_t, string_t>::value,
            bool> = true>
    value_t as() &&
    {
        value_t res;
        ext::jsonization<string_t, value_t>().move_from_json_object(std::move(*this), res);
        return res;
    }

    // Usage: get(key_1, key_2, ..., default_value);
    template <typename... key_then_default_value_t>
    auto get(key_then_default_value_t&&... keys_then_default_value) const;

    template <typename value_t = basic_value<string_t>>
    std::optional<value_t> find(const string_t& key) const;

    template <typename... args_t>
    decltype(auto) emplace(args_t&&... args);
    template <typename... args_t>
    decltype(auto) insert(args_t&&... args);

    void clear() noexcept;
    bool erase(const string_t& key);
    bool erase(iterator iter);

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    basic_value<string_t>& operator[](const string_t& key);
    basic_value<string_t>& operator[](string_t&& key);

    basic_object<string_t> operator|(const basic_object<string_t>& rhs) const&;
    basic_object<string_t> operator|(basic_object<string_t>&& rhs) const&;
    basic_object<string_t> operator|(const basic_object<string_t>& rhs) &&;
    basic_object<string_t> operator|(basic_object<string_t>&& rhs) &&;

    basic_object<string_t>& operator|=(const basic_object<string_t>& rhs);
    basic_object<string_t>& operator|=(basic_object<string_t>&& rhs);

    basic_object<string_t>& operator=(const basic_object<string_t>&) = default;
    basic_object<string_t>& operator=(basic_object<string_t>&&) = default;

    template <
        typename value_t,
        std::enable_if_t<std::is_convertible_v<value_t, basic_object<string_t>>, bool> = true>
    basic_object<string_t>& operator=(value_t rhs)
    {
        return *this = basic_object<string_t>(std::move(rhs));
    }

    bool operator==(const basic_object<string_t>& rhs) const;

    bool operator!=(const basic_object<string_t>& rhs) const { return !(*this == rhs); }

    template <
        typename value_t,
        template <typename...> typename map_t = std::map,
        std::enable_if_t<_utils::is_map<map_t<string_t, value_t>>, bool> = true>
    explicit operator map_t<string_t, value_t>() const
    {
        return as_map<value_t, map_t>();
    }

    template <
        typename jsonization_t,
        std::enable_if_t<
            _utils::has_from_json_in_templ_spec<jsonization_t, string_t>::value
                && !_utils::has_from_json_object_in_templ_spec<jsonization_t, string_t>::value,
            bool> = true>
    explicit operator jsonization_t() const&
    {
        jsonization_t dst {};
        if (!ext::jsonization<string_t, jsonization_t>().from_json(*this, dst)) {
            throw exception("Wrong JSON");
        }
        return dst;
    }

    template <
        typename jsonization_t,
        std::enable_if_t<
            _utils::has_from_json_object_in_templ_spec<jsonization_t, string_t>::value,
            bool> = true>
    explicit operator jsonization_t() const&
    {
        jsonization_t dst {};
        if (!ext::jsonization<string_t, jsonization_t>().from_json_object(*this, dst)) {
            throw exception("Wrong JSON");
        }
        return dst;
    }

    template <
        typename jsonization_t,
        std::enable_if_t<
            _utils::has_move_from_json_in_templ_spec<jsonization_t, string_t>::value
                && !_utils::has_move_from_json_object_in_templ_spec<jsonization_t, string_t>::value,
            bool> = true>
    explicit operator jsonization_t() &&
    {
        jsonization_t dst {};
        if (!ext::jsonization<string_t, jsonization_t>().from_json(std::move(*this), dst)) {
            throw exception("Wrong JSON");
        }
        return dst;
    }

    template <
        typename jsonization_t,
        std::enable_if_t<
            _utils::has_move_from_json_object_in_templ_spec<jsonization_t, string_t>::value,
            bool> = true>
    explicit operator jsonization_t() &&
    {
        jsonization_t dst {};
        if (!ext::jsonization<string_t, jsonization_t>().move_from_json_object(
                std::move(*this),
                dst)) {
            throw exception("Wrong JSON");
        }
        return dst;
    }

private:
    template <typename... key_then_default_value_t, size_t... keys_indexes_t>
    auto
        get(std::tuple<key_then_default_value_t...> keys_then_default_value,
            std::index_sequence<keys_indexes_t...>) const;
    template <typename value_t, typename... rest_keys_t>
    auto get_helper(const value_t& default_value, const string_t& key, rest_keys_t&&... rest) const;
    template <typename value_t>
    auto get_helper(const value_t& default_value, const string_t& key) const;

    string_t format(size_t indent, size_t indent_times) const;

private:
    raw_object _object_data;
};

template <typename string_t>
inline basic_object<string_t>::basic_object(std::initializer_list<value_type> init_list)
    : _object_data(
          std::make_move_iterator(init_list.begin()),
          std::make_move_iterator(init_list.end()))
{
}

// template <typename string_t>
// inline basic_object<string_t>::basic_object(const basic_value<string_t>& val) :
// basic_object<string_t>(val.as_object())
//{}
//
// template <typename string_t>
// inline basic_object<string_t>::basic_object(basic_value<string_t>&& val)
//     : basic_object<string_t>(std::move(val.as_object()))
//{}

template <typename string_t>
inline bool basic_object<string_t>::contains(const string_t& key) const
{
    return _object_data.find(key) != _object_data.cend();
}

template <typename string_t>
inline const basic_value<string_t>& basic_object<string_t>::at(const string_t& key) const
{
    return _object_data.at(key);
}

template <typename string_t>
inline void basic_object<string_t>::clear() noexcept
{
    _object_data.clear();
}

template <typename string_t>
inline bool basic_object<string_t>::erase(const string_t& key)
{
    return _object_data.erase(key) > 0 ? true : false;
}

template <typename string_t>
inline bool basic_object<string_t>::erase(iterator iter)
{
    return _object_data.erase(iter) != _object_data.end();
}

template <typename string_t>
template <typename... args_t>
inline decltype(auto) basic_object<string_t>::emplace(args_t&&... args)
{
    static_assert(
        std::is_constructible_v<value_type, args_t...>,
        "Parameter can't be used to construct a raw_object::value_type");
    return _object_data.insert_or_assign(std::forward<args_t>(args)...);
}

template <typename string_t>
template <typename... args_t>
inline decltype(auto) basic_object<string_t>::insert(args_t&&... args)
{
    return emplace(std::forward<args_t>(args)...);
}

template <typename string_t>
inline string_t basic_object<string_t>::to_string() const
{
    string_t str { '{' };
    for (auto iter = _object_data.cbegin(); iter != _object_data.cend();) {
        const auto& [key, val] = *iter;
        str +=
            char_t('"') + _utils::unescape_string(key) + string_t { '\"', ':' } + val.to_string();
        if (++iter != _object_data.cend()) {
            str += ',';
        }
    }
    str += char_t('}');
    return str;
}

template <typename string_t>
inline string_t basic_object<string_t>::format(size_t indent, size_t indent_times) const
{
    const string_t tail_indent(indent * indent_times, ' ');
    const string_t body_indent(indent * (indent_times + 1), ' ');

    string_t str { '{', '\n' };
    for (auto iter = _object_data.cbegin(); iter != _object_data.cend();) {
        const auto& [key, val] = *iter;
        str += body_indent + char_t('"') + _utils::unescape_string(key)
               + string_t { '\"', ':', ' ' } + val.format(indent, indent_times + 1);
        if (++iter != _object_data.cend()) {
            str += ',';
        }
        str += '\n';
    }
    str += tail_indent + char_t('}');
    return str;
}

template <typename string_t>
template <typename value_t>
inline bool basic_object<string_t>::all() const
{
    for (const auto& [_, val] : _object_data) {
        if (!val.template is<value_t>()) {
            return false;
        }
    }
    return true;
}

template <typename string_t>
template <typename value_t, template <typename...> typename map_t>
inline map_t<string_t, value_t> basic_object<string_t>::as_map() const
{
    return as<map_t<string_t, value_t>>();
}

template <typename string_t>
template <typename... key_then_default_value_t>
inline auto basic_object<string_t>::get(key_then_default_value_t&&... keys_then_default_value) const
{
    return get(
        std::forward_as_tuple(keys_then_default_value...),
        std::make_index_sequence<sizeof...(keys_then_default_value) - 1> {});
}

template <typename string_t>
template <typename... key_then_default_value_t, size_t... keys_indexes_t>
inline auto basic_object<string_t>::get(
    std::tuple<key_then_default_value_t...> keys_then_default_value,
    std::index_sequence<keys_indexes_t...>) const
{
    constexpr unsigned long default_value_index = sizeof...(key_then_default_value_t) - 1;
    return get_helper(
        std::get<default_value_index>(keys_then_default_value),
        std::get<keys_indexes_t>(keys_then_default_value)...);
}

template <typename string_t>
template <typename value_t, typename... rest_keys_t>
inline auto basic_object<string_t>::get_helper(
    const value_t& default_value,
    const string_t& key,
    rest_keys_t&&... rest) const
{
    constexpr bool is_json = std::is_same_v<basic_value<string_t>, value_t>
                             || std::is_same_v<basic_array<string_t>, value_t>
                             || std::is_same_v<basic_object<string_t>, value_t>;
    constexpr bool is_string = std::is_constructible_v<string_t, value_t> && !is_json;

    if (!contains(key)) {
        if constexpr (is_string) {
            return string_t(default_value);
        }
        else {
            return value_t(default_value);
        }
    }

    return at(key).get_helper(default_value, std::forward<rest_keys_t>(rest)...);
}

template <typename string_t>
template <typename value_t>
inline auto
    basic_object<string_t>::get_helper(const value_t& default_value, const string_t& key) const
{
    constexpr bool is_json = std::is_same_v<basic_value<string_t>, value_t>
                             || std::is_same_v<basic_array<string_t>, value_t>
                             || std::is_same_v<basic_object<string_t>, value_t>;
    constexpr bool is_string = std::is_constructible_v<string_t, value_t> && !is_json;

    if (!contains(key)) {
        if constexpr (is_string) {
            return string_t(default_value);
        }
        else {
            return value_t(default_value);
        }
    }

    auto val = _object_data.at(key);
    if (val.template is<value_t>()) {
        if constexpr (is_string) {
            return val.template as<string_t>();
        }
        else {
            return val.template as<value_t>();
        }
    }
    else {
        if constexpr (is_string) {
            return string_t(default_value);
        }
        else {
            return value_t(default_value);
        }
    }
}

template <typename string_t>
template <typename value_t>
inline std::optional<value_t> basic_object<string_t>::find(const string_t& key) const
{
    auto iter = _object_data.find(key);
    if (iter == _object_data.end()) {
        return std::nullopt;
    }
    const auto& val = iter->second;
    return val.template is<value_t>() ? std::optional<value_t>(val.template as<value_t>())
                                      : std::nullopt;
}

template <typename string_t>
inline typename basic_object<string_t>::iterator basic_object<string_t>::begin() noexcept
{
    return _object_data.begin();
}

template <typename string_t>
inline typename basic_object<string_t>::iterator basic_object<string_t>::end() noexcept
{
    return _object_data.end();
}

template <typename string_t>
inline typename basic_object<string_t>::const_iterator
    basic_object<string_t>::begin() const noexcept
{
    return _object_data.begin();
}

template <typename string_t>
inline typename basic_object<string_t>::const_iterator basic_object<string_t>::end() const noexcept
{
    return _object_data.end();
}

template <typename string_t>
inline typename basic_object<string_t>::const_iterator
    basic_object<string_t>::cbegin() const noexcept
{
    return _object_data.cbegin();
}

template <typename string_t>
inline typename basic_object<string_t>::const_iterator basic_object<string_t>::cend() const noexcept
{
    return _object_data.cend();
}

template <typename string_t>
inline basic_value<string_t>& basic_object<string_t>::operator[](const string_t& key)
{
    return _object_data[key];
}

template <typename string_t>
inline basic_value<string_t>& basic_object<string_t>::operator[](string_t&& key)
{
    return _object_data[std::move(key)];
}

template <typename string_t>
inline basic_object<string_t>
    basic_object<string_t>::operator|(const basic_object<string_t>& rhs) const&
{
    basic_object<string_t> temp = *this;
    temp._object_data.insert(rhs.begin(), rhs.end());
    return temp;
}

template <typename string_t>
inline basic_object<string_t> basic_object<string_t>::operator|(basic_object<string_t>&& rhs) const&
{
    basic_object<string_t> temp = *this;
    // temp._object_data.merge(std::move(rhs._object_data));
    temp._object_data.insert(
        std::make_move_iterator(rhs.begin()),
        std::make_move_iterator(rhs.end()));
    return temp;
}

template <typename string_t>
inline basic_object<string_t>
    basic_object<string_t>::operator|(const basic_object<string_t>& rhs) &&
{
    _object_data.insert(rhs.begin(), rhs.end());
    return std::move(*this);
}

template <typename string_t>
inline basic_object<string_t> basic_object<string_t>::operator|(basic_object<string_t>&& rhs) &&
{
    //_object_data.merge(std::move(rhs._object_data));
    _object_data.insert(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
    return std::move(*this);
}

template <typename string_t>
inline basic_object<string_t>& basic_object<string_t>::operator|=(const basic_object<string_t>& rhs)
{
    _object_data.insert(rhs.begin(), rhs.end());
    return *this;
}

template <typename string_t>
inline basic_object<string_t>& basic_object<string_t>::operator|=(basic_object<string_t>&& rhs)
{
    _object_data.insert(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
    return *this;
}

template <typename string_t>
inline bool basic_object<string_t>::operator==(const basic_object<string_t>& rhs) const
{
    return _object_data == rhs._object_data;
}

template <
    typename ostream_t,
    typename string_t,
    typename std_ostream_t = std::basic_ostream<
        typename string_t::value_type,
        std::char_traits<typename string_t::value_type>>,
    typename enable_t = std::enable_if_t<
        std::is_same_v<std_ostream_t, ostream_t> || std::is_base_of_v<std_ostream_t, ostream_t>>>
ostream_t& operator<<(ostream_t& out, const basic_object<string_t>& obj)
{
    out << obj.format();
    return out;
}
} // namespace json
