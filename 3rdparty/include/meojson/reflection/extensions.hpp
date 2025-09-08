// IWYU pragma: private, include <meojson/json.hpp>

#pragma once

#include <cstddef>
#include <filesystem>
#include <queue>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "../common/types.hpp"

namespace json::ext
{

template <typename string_t, typename impl_t, typename var_t, size_t len> // (size_t)-1 for no
                                                                          // restriction
class __jsonization_array
{
public:
    json::basic_value<string_t> to_json(const var_t& value) const
    {
        return static_cast<const impl_t*>(this)->to_json_array(value);
    }

    bool check_json(const json::basic_value<string_t>& json) const
    {
        if (!json.is_array()) {
            return false;
        }
        const auto& arr = json.as_array();
        if constexpr (len != static_cast<size_t>(-1)) {
            if (len != arr.size()) {
                return false;
            }
        }
        return static_cast<const impl_t*>(this)->check_json_array(arr);
    }

    bool from_json(const json::basic_value<string_t>& json, var_t& value) const
    {
        if (!json.is_array()) {
            return false;
        }
        const auto& arr = json.as_array();
        if constexpr (len != static_cast<size_t>(-1)) {
            if (len != arr.size()) {
                return false;
            }
        }
        return static_cast<const impl_t*>(this)->from_json_array(arr, value);
    }

    json::basic_value<string_t> move_to_json(var_t value) const
    {
        return static_cast<const impl_t*>(this)->move_to_json_array(std::move(value));
    }

    bool move_from_json(json::basic_value<string_t> json, var_t& value) const
    {
        if (!json.is_array()) {
            return false;
        }
        auto& arr = json.as_array();
        if constexpr (len != static_cast<size_t>(-1)) {
            if (len != arr.size()) {
                return false;
            }
        }
        return static_cast<const impl_t*>(this)->move_from_json_array(std::move(arr), value);
    }
};

template <typename string_t, typename impl_t, typename var_t>
class __jsonization_object
{
public:
    json::basic_value<string_t> to_json(const var_t& value) const
    {
        return static_cast<const impl_t*>(this)->to_json_object(value);
    }

    bool check_json(const json::basic_value<string_t>& json) const
    {
        if (!json.is_object()) {
            return false;
        }
        const auto& obj = json.as_object();
        return static_cast<const impl_t*>(this)->check_json_object(obj);
    }

    bool from_json(const json::basic_value<string_t>& json, var_t& value) const
    {
        if (!json.is_object()) {
            return false;
        }
        const auto& obj = json.as_object();
        return static_cast<const impl_t*>(this)->from_json_object(obj, value);
    }

    json::basic_value<string_t> move_to_json(var_t value) const
    {
        return static_cast<const impl_t*>(this)->move_to_json_object(std::move(value));
    }

    bool move_from_json(json::basic_value<string_t> json, var_t& value) const
    {
        if (!json.is_object()) {
            return false;
        }
        auto& obj = json.as_object();
        return static_cast<const impl_t*>(this)->move_from_json_object(std::move(obj), value);
    }
};

template <typename string_t>
class jsonization<string_t, std::nullptr_t>
{
public:
    json::basic_value<string_t> to_json(const std::nullptr_t&) const
    {
        return json::basic_value<string_t> {};
    }

    bool check_json(const json::basic_value<string_t>& json) const { return json.is_null(); }

    bool from_json(const json::basic_value<string_t>& json, std::nullptr_t&)
    {
        return check_json(json);
    }
};

template <typename string_t>
class jsonization<
    string_t,
    std::filesystem::path,
    std::enable_if_t<
        std::is_same_v<string_t, std::filesystem::path::string_type>
        || std::is_same_v<string_t, std::string>>>
{
public:
    json::basic_value<string_t> to_json(const std::filesystem::path& path) const
    {
        if constexpr (std::is_same_v<string_t, std::filesystem::path::string_type>) {
            return path.native();
        }
        else if constexpr (std::is_same_v<string_t, std::string>) {
#if __cplusplus >= 202002L
            std::u8string u8str = path.u8string();
            return std::string { u8str.begin(), u8str.end() };
#else
            return path.u8string();
#endif
        }
#if __cplusplus >= 202002L
        else if constexpr (std::is_same_v<string_t, std::u8string>) {
            return path.u8string();
        }
#endif
    }

    bool check_json(const json::basic_value<string_t>& json) const { return json.is_string(); }

    bool from_json(const json::basic_value<string_t>& json, std::filesystem::path& path) const
    {
        path = json.as_string();
        return true;
    }
};

template <
    typename string_t,
    template <typename, size_t> typename arr_t,
    typename value_t,
    size_t size>
class jsonization<string_t, arr_t<value_t, size>>
    : public __jsonization_array<
          string_t,
          jsonization<string_t, arr_t<value_t, size>>,
          arr_t<value_t, size>,
          size>
{
public:
    json::basic_array<string_t> to_json_array(const arr_t<value_t, size>& value) const
    {
        json::basic_array<string_t> result;
        for (size_t i = 0; i < size; i++) {
            result.emplace_back(value.at(i));
        }
        return result;
    }

    bool check_json_array(const json::basic_array<string_t>& arr) const
    {
        return arr.template all<value_t>();
    }

    bool from_json_array(const json::basic_array<string_t>& arr, arr_t<value_t, size>& value) const
    {
        if (!check_json_array(arr)) {
            return false;
        }

        for (size_t i = 0; i < size; i++) {
            value.at(i) = arr[i].template as<value_t>();
        }
        return true;
    }

    json::basic_array<string_t> move_to_json_array(arr_t<value_t, size> value) const
    {
        json::basic_array<string_t> result;
        for (size_t i = 0; i < size; i++) {
            result.emplace_back(std::move(value.at(i)));
        }
        return result;
    }

    bool move_from_json_array(json::basic_array<string_t> arr, arr_t<value_t, size>& value) const
    {
        if (!check_json_array(arr)) {
            return false;
        }

        for (size_t i = 0; i < size; i++) {
            value.at(i) = std::move(arr[i]).template as<value_t>();
        }
        return true;
    }
};

template <typename string_t, typename collection_t>
class jsonization<string_t, collection_t, std::enable_if_t<_utils::is_collection<collection_t>>>
    : public __jsonization_array<
          string_t,
          jsonization<string_t, collection_t>,
          collection_t,
          (size_t)-1>
{
public:
    json::basic_array<string_t> to_json_array(const collection_t& value) const
    {
        json::basic_array<string_t> result;
        for (const auto& val : value) {
            result.emplace_back(val);
        }
        return result;
    }

    bool check_json_array(const json::basic_array<string_t>& arr) const
    {
        return arr.template all<typename collection_t::value_type>();
    }

    bool from_json_array(const json::basic_array<string_t>& arr, collection_t& value) const
    {
        if (!check_json_array(arr)) {
            return false;
        }

        value = {};
        for (const auto& val : arr) {
            if constexpr (_utils::has_emplace_back<collection_t>::value) {
                value.emplace_back(val.template as<typename collection_t::value_type>());
            }
            else {
                value.emplace(val.template as<typename collection_t::value_type>());
            }
        }
        return true;
    }

    json::basic_array<string_t> move_to_json_array(collection_t value) const
    {
        json::basic_array<string_t> result;
        for (auto& val : value) {
            result.emplace_back(std::move(val));
        }
        return result;
    }

    bool move_from_json_array(json::basic_array<string_t> arr, collection_t& value) const
    {
        if (!check_json_array(arr)) {
            return false;
        }

        for (auto& val : arr) {
            if constexpr (_utils::has_emplace_back<collection_t>::value) {
                value.emplace_back(std::move(val).template as<typename collection_t::value_type>());
            }
            else {
                value.emplace(std::move(val).template as<typename collection_t::value_type>());
            }
        }
        return true;
    }
};

template <typename string_t, template <typename...> typename tuple_t, typename... args_t>
class jsonization<
    string_t,
    tuple_t<args_t...>,
    std::enable_if_t<_utils::is_tuple_like<tuple_t<args_t...>>>>
    : public __jsonization_array<
          string_t,
          jsonization<string_t, tuple_t<args_t...>>,
          tuple_t<args_t...>,
          std::tuple_size_v<tuple_t<args_t...>>>
{
public:
    constexpr static size_t tuple_size = std::tuple_size_v<tuple_t<args_t...>>;

    json::basic_array<string_t> to_json_array(const tuple_t<args_t...>& value) const
    {
        json::basic_array<string_t> result;
        to_json_impl(result, value, std::make_index_sequence<tuple_size>());
        return result;
    }

    template <std::size_t... Is>
    void to_json_impl(
        json::basic_array<string_t>& arr,
        const tuple_t<args_t...>& t,
        std::index_sequence<Is...>) const
    {
        using std::get;
        (arr.emplace_back(get<Is>(t)), ...);
    }

    bool check_json_array(const json::basic_array<string_t>& arr) const
    {
        return check_json_impl(arr, std::make_index_sequence<tuple_size>());
    }

    template <std::size_t... Is>
    bool check_json_impl(const json::basic_array<string_t>& arr, std::index_sequence<Is...>) const
    {
        return (arr[Is].template is<std::tuple_element_t<Is, tuple_t<args_t...>>>() && ...);
    }

    bool from_json_array(const json::basic_array<string_t>& arr, tuple_t<args_t...>& value) const
    {
        if (!check_json_array(arr)) {
            return false;
        }

        from_json_impl(arr, value, std::make_index_sequence<tuple_size>());
        return true;
    }

    template <std::size_t... Is>
    void from_json_impl(
        const json::basic_array<string_t>& arr,
        tuple_t<args_t...>& t,
        std::index_sequence<Is...>) const
    {
        using std::get;
        ((get<Is>(t) = arr[Is].template as<std::tuple_element_t<Is, tuple_t<args_t...>>>()), ...);
    }

    json::basic_array<string_t> move_to_json_array(tuple_t<args_t...> value) const
    {
        json::basic_array<string_t> result;
        move_to_json_impl(result, std::move(value), std::make_index_sequence<tuple_size>());
        return result;
    }

    template <std::size_t... Is>
    void move_to_json_impl(
        json::basic_array<string_t>& arr,
        tuple_t<args_t...> t,
        std::index_sequence<Is...>) const
    {
        using std::get;
        (arr.emplace_back(std::move(get<Is>(t))), ...);
    }

    bool move_from_json_array(json::basic_array<string_t> arr, tuple_t<args_t...>& value) const
    {
        if (!check_json_array(arr)) {
            return false;
        }

        move_from_json_impl(arr, value, std::make_index_sequence<tuple_size>());
        return true;
    }

    template <std::size_t... Is>
    void move_from_json_impl(
        json::basic_array<string_t> arr,
        tuple_t<args_t...>& t,
        std::index_sequence<Is...>) const
    {
        using std::get;
        ((get<Is>(t) =
              std::move(arr[Is]).template as<std::tuple_element_t<Is, tuple_t<args_t...>>>()),
         ...);
    }
};

template <typename string_t, typename map_t>
class jsonization<
    string_t,
    map_t,
    std::enable_if_t<_utils::is_map<map_t> && std::is_same_v<typename map_t::key_type, string_t>>>
    : public __jsonization_object<string_t, jsonization<string_t, map_t>, map_t>
{
public:
    json::basic_object<string_t> to_json_object(const map_t& value) const
    {
        json::basic_object<string_t> result;
        for (const auto& [key, val] : value) {
            result.emplace(key, val);
        }
        return result;
    }

    bool check_json_object(const json::basic_object<string_t>& arr) const
    {
        for (const auto& [key, val] : arr) {
            if (!val.template is<typename map_t::mapped_type>()) {
                return false;
            }
        }
        return true;
    }

    bool from_json_object(const json::basic_object<string_t>& arr, map_t& value) const
    {
        // TODO: 是不是直接from不check了算了
        if (!check_json_object(arr)) {
            return false;
        }

        value = {};
        for (const auto& [key, val] : arr) {
            value.emplace(key, val.template as<typename map_t::mapped_type>());
        }
        return true;
    }

    json::basic_object<string_t> move_to_json_object(map_t value) const
    {
        json::basic_object<string_t> result;
        for (auto& [key, val] : value) {
            result.emplace(key, std::move(val));
        }
        return result;
    }

    bool move_from_json_object(json::basic_object<string_t> arr, map_t& value) const
    {
        // TODO: 是不是直接from不check了算了
        if (!check_json_object(arr)) {
            return false;
        }

        value = {};
        for (auto& [key, val] : arr) {
            value.emplace(key, std::move(val).template as<typename map_t::mapped_type>());
        }
        return true;
    }
};

template <typename string_t, typename... args_t>
class jsonization<string_t, std::variant<args_t...>>
{
public:
    using variant_t = std::variant<args_t...>;
    constexpr static size_t variant_size = std::variant_size_v<variant_t>;

    json::basic_value<string_t> to_json(const variant_t& value) const
    {
        json::basic_value<string_t> result;
        to_json_impl(result, value, std::make_index_sequence<variant_size>());
        return result;
    }

    template <std::size_t... Is>
    void to_json_impl(
        json::basic_value<string_t>& val,
        const variant_t& t,
        std::index_sequence<Is...>) const
    {
        using std::get;
        std::ignore = ((t.index() == Is ? (val = get<Is>(t), true) : false) || ...);
    }

    bool check_json(const json::basic_value<string_t>& json) const
    {
        return check_json_impl(json, std::make_index_sequence<variant_size>());
    }

    template <std::size_t... Is>
    bool check_json_impl(const json::basic_value<string_t>& val, std::index_sequence<Is...>) const
    {
        return (val.template is<std::variant_alternative_t<Is, variant_t>>() || ...);
    }

    bool from_json(const json::basic_value<string_t>& json, variant_t& value) const
    {
        if (!check_json_impl(json, std::make_index_sequence<variant_size>())) {
            return false;
        }

        from_json_impl(json, value, std::make_index_sequence<variant_size>());
        return true;
    }

    template <std::size_t... Is>
    void from_json_impl(
        const json::basic_value<string_t>& json,
        variant_t& t,
        std::index_sequence<Is...>) const
    {
        std::ignore =
            ((json.template is<std::variant_alternative_t<Is, variant_t>>()
                  ? (t = json.template as<std::variant_alternative_t<Is, variant_t>>(), true)
                  : false)
             || ...);
    }

    json::basic_value<string_t> move_to_json(variant_t value) const
    {
        json::basic_value<string_t> result;
        move_to_json_impl(result, std::move(value), std::make_index_sequence<variant_size>());
        return result;
    }

    template <std::size_t... Is>
    void
        move_to_json_impl(json::basic_value<string_t>& val, variant_t t, std::index_sequence<Is...>)
            const
    {
        using std::get;
        std::ignore = ((t.index() == Is ? (val = std::move(get<Is>(t)), true) : false) || ...);
    }

    bool move_from_json(json::basic_value<string_t> json, variant_t& value) const
    {
        if (!check_json_impl(json, std::make_index_sequence<variant_size>())) {
            return false;
        }

        move_from_json_impl(std::move(json), value, std::make_index_sequence<variant_size>());
        return true;
    }

    template <std::size_t... Is>
    void move_from_json_impl(
        json::basic_value<string_t> json,
        variant_t& t,
        std::index_sequence<Is...>) const
    {
        std::ignore =
            ((json.template is<std::variant_alternative_t<Is, variant_t>>()
                  ? (t = std::move(json).template as<std::variant_alternative_t<Is, variant_t>>(),
                     true)
                  : false)
             || ...);
    }
};

// TODO: check if has move_xxx in member
template <typename string_t, typename var_t>
class jsonization<
    string_t,
    var_t,
    std::enable_if_t<
        _utils::has_to_json_in_member<var_t>::value
        && _utils::has_check_json_in_member<var_t, string_t>::value
        && _utils::has_from_json_in_member<var_t, string_t>::value>>
{
public:
    json::basic_value<string_t> to_json(const var_t& value) const { return value.to_json(); }

    bool check_json(const json::basic_value<string_t>& json) const
    {
        var_t value;
        return value.check_json(json);
    }

    bool from_json(const json::basic_value<string_t>& json, var_t& value) const
    {
        return value.from_json(json);
    }

    json::basic_value<string_t> move_to_json(var_t value) const { return to_json(value); }

    bool move_from_json(json::basic_value<string_t> json, var_t& value) const
    {
        return from_json(json, value);
    }
};

// really need this fucking queue?
template <typename string_t, typename value_t>
class jsonization<string_t, std::queue<value_t>>
    : public __jsonization_array<
          string_t,
          jsonization<string_t, std::queue<value_t>>,
          std::queue<value_t>,
          (size_t)-1>
{
public:
    json::basic_array<string_t> to_json_array(const std::queue<value_t>& value) const
    {
        return move_to_json_array(value);
    }

    bool check_json_array(const json::basic_array<string_t>& arr) const
    {
        return arr.template all<value_t>();
    }

    bool from_json_array(const json::basic_array<string_t>& arr, std::queue<value_t>& value) const
    {
        if (!check_json_array(arr)) {
            return false;
        }

        value = {};
        for (const auto& val : arr) {
            value.emplace(val.template as<value_t>());
        }
        return true;
    }

    json::basic_array<string_t> move_to_json_array(std::queue<value_t> value) const
    {
        json::basic_array<string_t> result;
        while (!value.empty()) {
            result.emplace_back(std::move(value.front()));
            value.pop();
        }
        return result;
    }

    bool move_from_json_array(json::basic_array<string_t> arr, std::queue<value_t>& value) const
    {
        if (!check_json_array(arr)) {
            return false;
        }

        for (auto& val : arr) {
            value.emplace(std::move(val).template as<value_t>());
        }
        return true;
    }
};

}
