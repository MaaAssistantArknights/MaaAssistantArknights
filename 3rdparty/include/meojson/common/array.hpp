#pragma once

#include <initializer_list>
#include <optional>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>

#include "exception.hpp"
#include "utils.hpp"

namespace json
{
template <typename string_t>
class basic_array
{
    friend class basic_value<string_t>;
    friend class basic_object<string_t>;

public:
    using raw_array = std::vector<basic_value<string_t>>;
    using value_type = typename raw_array::value_type;
    using iterator = typename raw_array::iterator;
    using const_iterator = typename raw_array::const_iterator;
    using reverse_iterator = typename raw_array::reverse_iterator;
    using const_reverse_iterator = typename raw_array::const_reverse_iterator;
    using char_t = typename string_t::value_type;

public:
    basic_array() = default;
    basic_array(const basic_array<string_t>& rhs) = default;
    basic_array(basic_array<string_t>&& rhs) noexcept = default;
    basic_array(std::initializer_list<value_type> init_list);
    basic_array(typename raw_array::size_type size);

    // explicit basic_array(const basic_value<string_t>& val);
    // explicit basic_array(basic_value<string_t>&& val);

    template <typename collection_t,
              std::enable_if_t<_utils::is_collection<collection_t> &&
                                   std::is_constructible_v<value_type, _utils::range_value_t<collection_t>>,
                               bool> = true>
    basic_array(collection_t arr)
        : _array_data(std::make_move_iterator(arr.begin()), std::make_move_iterator(arr.end()))
    {}
    template <typename jsonization_t,
              std::enable_if_t<_utils::has_to_json_in_member<jsonization_t>::value, bool> = true>
    basic_array(const jsonization_t& value) : basic_array(value.to_json())
    {}
    template <typename jsonization_t,
              std::enable_if_t<_utils::has_to_json_in_templ_spec<jsonization_t>::value, bool> = true>
    basic_array(const jsonization_t& value) : basic_array(ext::jsonization<jsonization_t>().to_json(value))
    {}

    ~basic_array() noexcept = default;

    bool empty() const noexcept { return _array_data.empty(); }
    size_t size() const noexcept { return _array_data.size(); }
    bool contains(size_t pos) const { return pos < _array_data.size(); }
    bool exists(size_t pos) const { return contains(pos); }
    const basic_value<string_t>& at(size_t pos) const;

    string_t dumps(std::optional<size_t> indent = std::nullopt) const { return indent ? format(*indent) : to_string(); }
    string_t to_string() const;
    string_t format(size_t indent = 4) const { return format(indent, 0); }
    template <typename value_t>
    bool all() const;
    template <typename value_t, template <typename...> typename collection_t = std::vector>
    collection_t<value_t> as_collection() const;

    // Usage: get(key_1, key_2, ..., default_value);
    template <typename... key_then_default_value_t>
    auto get(key_then_default_value_t&&... keys_then_default_value) const;

    template <typename value_t = basic_value<string_t>>
    std::optional<value_t> find(size_t pos) const;

    template <typename... args_t>
    decltype(auto) emplace_back(args_t&&... args);
    template <typename... args_t>
    decltype(auto) push_back(args_t&&... args);

    void clear() noexcept;
    bool erase(size_t pos);
    bool erase(iterator iter);

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    reverse_iterator rbegin() noexcept;
    reverse_iterator rend() noexcept;
    const_reverse_iterator rbegin() const noexcept;
    const_reverse_iterator rend() const noexcept;
    const_reverse_iterator crbegin() const noexcept;
    const_reverse_iterator crend() const noexcept;

    const basic_value<string_t>& operator[](size_t pos) const;
    basic_value<string_t>& operator[](size_t pos);

    basic_array<string_t> operator+(const basic_array<string_t>& rhs) const&;
    basic_array<string_t> operator+(basic_array<string_t>&& rhs) const&;
    basic_array<string_t> operator+(const basic_array<string_t>& rhs) &&;
    basic_array<string_t> operator+(basic_array<string_t>&& rhs) &&;

    basic_array<string_t>& operator+=(const basic_array<string_t>& rhs);
    basic_array<string_t>& operator+=(basic_array<string_t>&& rhs);

    basic_array<string_t>& operator=(const basic_array<string_t>&) = default;
    basic_array<string_t>& operator=(basic_array<string_t>&&) noexcept = default;
    template <typename value_t, std::enable_if_t<std::is_convertible_v<value_t, basic_array<string_t>>, bool> = true>
    basic_array<string_t>& operator=(value_t rhs)
    {
        return *this = basic_array<string_t>(std::move(rhs));
    }

    bool operator==(const basic_array<string_t>& rhs) const;
    bool operator!=(const basic_array<string_t>& rhs) const { return !(*this == rhs); }

    template <typename value_t, template <typename...> typename collection_t = std::vector,
              std::enable_if_t<_utils::is_collection<collection_t<value_t>>, bool> = true>
    explicit operator collection_t<value_t>() const
    {
        return as_collection<value_t, collection_t>();
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
    template <typename... key_then_default_value_t, size_t... keys_indexes_t>
    auto get(std::tuple<key_then_default_value_t...> keys_then_default_value,
             std::index_sequence<keys_indexes_t...>) const;
    template <typename value_t, typename... rest_keys_t>
    auto get_helper(const value_t& default_value, size_t pos, rest_keys_t&&... rest) const;
    template <typename value_t>
    auto get_helper(const value_t& default_value, size_t pos) const;

    string_t format(size_t indent, size_t indent_times) const;

private:
    raw_array _array_data;
};

template <typename string_t>
inline basic_array<string_t>::basic_array(std::initializer_list<value_type> init_list) : _array_data(init_list)
{}

template <typename string_t>
inline basic_array<string_t>::basic_array(typename raw_array::size_type size) : _array_data(size)
{}

// template <typename string_t>
// inline basic_array<string_t>::basic_array(const basic_value<string_t>& val) : basic_array<string_t>(val.as_array())
//{}
//
// template <typename string_t>
// inline basic_array<string_t>::basic_array(basic_value<string_t>&& val)
//     : basic_array<string_t>(std::move(val.as_array()))
//{}

template <typename string_t>
inline void basic_array<string_t>::clear() noexcept
{
    _array_data.clear();
}

template <typename string_t>
inline bool basic_array<string_t>::erase(size_t pos)
{
    return erase(_array_data.begin() + pos);
}

template <typename string_t>
inline bool basic_array<string_t>::erase(iterator iter)
{
    return _array_data.erase(iter) != _array_data.end();
}

template <typename string_t>
template <typename... args_t>
inline decltype(auto) basic_array<string_t>::emplace_back(args_t&&... args)
{
    static_assert(std::is_constructible_v<value_type, args_t...>,
                  "Parameter can't be used to construct a raw_array::value_type");
    return _array_data.emplace_back(std::forward<args_t>(args)...);
}

template <typename string_t>
template <typename... args_t>
inline decltype(auto) basic_array<string_t>::push_back(args_t&&... args)
{
    return emplace_back(std::forward<args_t>(args)...);
}

template <typename string_t>
inline const basic_value<string_t>& basic_array<string_t>::at(size_t pos) const
{
    return _array_data.at(pos);
}

template <typename string_t>
inline string_t basic_array<string_t>::to_string() const
{
    string_t str { '[' };
    for (auto iter = _array_data.cbegin(); iter != _array_data.cend();) {
        str += iter->to_string();
        if (++iter != _array_data.cend()) {
            str += ',';
        }
    }
    str += char_t(']');
    return str;
}

template <typename string_t>
inline string_t basic_array<string_t>::format(size_t indent, size_t indent_times) const
{
    const string_t tail_indent(indent * indent_times, ' ');
    const string_t body_indent(indent * (indent_times + 1), ' ');

    string_t str { '[', '\n' };
    for (auto iter = _array_data.cbegin(); iter != _array_data.cend();) {
        str += body_indent + iter->format(indent, indent_times + 1);
        if (++iter != _array_data.cend()) {
            str += ',';
        }
        str += '\n';
    }
    str += tail_indent + char_t(']');
    return str;
}

template <typename string_t>
template <typename value_t>
inline bool basic_array<string_t>::all() const
{
    for (const auto& elem : _array_data) {
        if (!elem.template is<value_t>()) {
            return false;
        }
    }
    return true;
}

namespace _as_collection_helper
{
    template <typename T>
    class has_emplace_back
    {
        template <typename U>
        static auto test(int) -> decltype(std::declval<U>().emplace_back(), std::true_type());

        template <typename U>
        static std::false_type test(...);

    public:
        static constexpr bool value = decltype(test<T>(0))::value;
    };
}

template <typename string_t>
template <typename value_t, template <typename...> typename collection_t>
inline collection_t<value_t> basic_array<string_t>::as_collection() const
{
    collection_t<value_t> result;
    if constexpr (_as_collection_helper::has_emplace_back<collection_t<value_t>>::value) {
        for (const auto& elem : _array_data) {
            result.emplace_back(elem.template as<value_t>());
        }
    }
    else {
        for (const auto& elem : _array_data) {
            result.emplace(elem.template as<value_t>());
        }
    }
    return result;
}

template <typename string_t>
template <typename... key_then_default_value_t>
inline auto basic_array<string_t>::get(key_then_default_value_t&&... keys_then_default_value) const
{
    return get(std::forward_as_tuple(keys_then_default_value...),
               std::make_index_sequence<sizeof...(keys_then_default_value) - 1> {});
}

template <typename string_t>
template <typename... key_then_default_value_t, size_t... keys_indexes_t>
inline auto basic_array<string_t>::get(std::tuple<key_then_default_value_t...> keys_then_default_value,
                                       std::index_sequence<keys_indexes_t...>) const
{
    constexpr unsigned long default_value_index = sizeof...(key_then_default_value_t) - 1;
    return get_helper(std::get<default_value_index>(keys_then_default_value),
                      std::get<keys_indexes_t>(keys_then_default_value)...);
}

template <typename string_t>
template <typename value_t, typename... rest_keys_t>
inline auto basic_array<string_t>::get_helper(const value_t& default_value, size_t pos, rest_keys_t&&... rest) const
{
    constexpr bool is_json = std::is_same_v<basic_value<string_t>, value_t> ||
                             std::is_same_v<basic_array<string_t>, value_t> ||
                             std::is_same_v<basic_object<string_t>, value_t>;
    constexpr bool is_string = std::is_constructible_v<string_t, value_t> && !is_json;

    if (!contains(pos)) {
        if constexpr (is_string) {
            return string_t(default_value);
        }
        else {
            return value_t(default_value);
        }
    }

    return at(pos).get_helper(default_value, std::forward<rest_keys_t>(rest)...);
}

template <typename string_t>
template <typename value_t>
inline auto basic_array<string_t>::get_helper(const value_t& default_value, size_t pos) const
{
    constexpr bool is_json = std::is_same_v<basic_value<string_t>, value_t> ||
                             std::is_same_v<basic_array<string_t>, value_t> ||
                             std::is_same_v<basic_object<string_t>, value_t>;
    constexpr bool is_string = std::is_constructible_v<string_t, value_t> && !is_json;

    if (!contains(pos)) {
        if constexpr (is_string) {
            return string_t(default_value);
        }
        else {
            return value_t(default_value);
        }
    }

    auto val = _array_data.at(pos);
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
inline std::optional<value_t> basic_array<string_t>::find(size_t pos) const
{
    if (!contains(pos)) {
        return std::nullopt;
    }
    const auto& val = _array_data.at(pos);
    return val.template is<value_t>() ? std::optional<value_t>(val.template as<value_t>()) : std::nullopt;
}

template <typename string_t>
inline typename basic_array<string_t>::iterator basic_array<string_t>::begin() noexcept
{
    return _array_data.begin();
}

template <typename string_t>
inline typename basic_array<string_t>::iterator basic_array<string_t>::end() noexcept
{
    return _array_data.end();
}

template <typename string_t>
inline typename basic_array<string_t>::const_iterator basic_array<string_t>::begin() const noexcept
{
    return _array_data.begin();
}

template <typename string_t>
inline typename basic_array<string_t>::const_iterator basic_array<string_t>::end() const noexcept
{
    return _array_data.end();
}

template <typename string_t>
inline typename basic_array<string_t>::const_iterator basic_array<string_t>::cbegin() const noexcept
{
    return _array_data.cbegin();
}

template <typename string_t>
inline typename basic_array<string_t>::const_iterator basic_array<string_t>::cend() const noexcept
{
    return _array_data.cend();
}

template <typename string_t>
inline typename basic_array<string_t>::reverse_iterator basic_array<string_t>::rbegin() noexcept
{
    return _array_data.rbegin();
}

template <typename string_t>
inline typename basic_array<string_t>::reverse_iterator basic_array<string_t>::rend() noexcept
{
    return _array_data.rend();
}

template <typename string_t>
inline typename basic_array<string_t>::const_reverse_iterator basic_array<string_t>::rbegin() const noexcept
{
    return _array_data.rbegin();
}

template <typename string_t>
inline typename basic_array<string_t>::const_reverse_iterator basic_array<string_t>::rend() const noexcept
{
    return _array_data.rend();
}

template <typename string_t>
inline typename basic_array<string_t>::const_reverse_iterator basic_array<string_t>::crbegin() const noexcept
{
    return _array_data.crbegin();
}

template <typename string_t>
inline typename basic_array<string_t>::const_reverse_iterator basic_array<string_t>::crend() const noexcept
{
    return _array_data.crend();
}

template <typename string_t>
inline basic_value<string_t>& basic_array<string_t>::operator[](size_t pos)
{
    return _array_data[pos];
}

template <typename string_t>
inline const basic_value<string_t>& basic_array<string_t>::operator[](size_t pos) const
{
    return _array_data[pos];
}

template <typename string_t>
inline basic_array<string_t> basic_array<string_t>::operator+(const basic_array<string_t>& rhs) const&
{
    basic_array<string_t> temp = *this;
    temp._array_data.insert(_array_data.end(), rhs.begin(), rhs.end());
    return temp;
}

template <typename string_t>
inline basic_array<string_t> basic_array<string_t>::operator+(basic_array<string_t>&& rhs) const&
{
    basic_array<string_t> temp = *this;
    temp._array_data.insert(_array_data.end(), std::make_move_iterator(rhs.begin()),
                            std::make_move_iterator(rhs.end()));
    return temp;
}

template <typename string_t>
inline basic_array<string_t> basic_array<string_t>::operator+(const basic_array<string_t>& rhs) &&
{
    _array_data.insert(_array_data.end(), rhs.begin(), rhs.end());
    return std::move(*this);
}

template <typename string_t>
inline basic_array<string_t> basic_array<string_t>::operator+(basic_array<string_t>&& rhs) &&
{
    _array_data.insert(_array_data.end(), std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
    return std::move(*this);
}

template <typename string_t>
inline basic_array<string_t>& basic_array<string_t>::operator+=(const basic_array<string_t>& rhs)
{
    _array_data.insert(_array_data.end(), rhs.begin(), rhs.end());
    return *this;
}

template <typename string_t>
inline basic_array<string_t>& basic_array<string_t>::operator+=(basic_array<string_t>&& rhs)
{
    _array_data.insert(_array_data.end(), std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
    return *this;
}

template <typename string_t>
inline bool basic_array<string_t>::operator==(const basic_array<string_t>& rhs) const
{
    return _array_data == rhs._array_data;
}

template <typename ostream_t, typename string_t,
          typename std_ostream_t =
              std::basic_ostream<typename string_t::value_type, std::char_traits<typename string_t::value_type>>,
          typename enable_t =
              std::enable_if_t<std::is_same_v<std_ostream_t, ostream_t> || std::is_base_of_v<std_ostream_t, ostream_t>>>
ostream_t& operator<<(ostream_t& out, const basic_array<string_t>& arr)
{
    out << arr.format();
    return out;
}
} // namespace json
