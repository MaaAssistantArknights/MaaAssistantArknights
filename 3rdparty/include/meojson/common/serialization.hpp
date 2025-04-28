// IWYU pragma: private, include <meojson/json.hpp>

#pragma once

#include <type_traits>

#include "types.hpp"
#include "utils.hpp"

namespace json
{
namespace _serialization_helper
{
template <typename in_t, typename serializer_t>
class is_serializable
{
    template <typename U>
    static auto test(int)
        -> decltype(std::declval<serializer_t>()(std::declval<U>()), std::true_type());

    template <typename U>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<in_t>(0))::value;
};

struct empty_serializer
{
    // sample:
    // json::value operator()(const type_1&) const { return ...; }
    // json::value operator()(const type_2&) const { return ...; }
    // json::value operator()(const type_3&) const { return ...; }
};

template <typename T>
void unable_to_serialize()
{
    static_assert(
        !sizeof(T),
        "Unable to serialize T. "
#ifdef _MSC_VER
        "See T below: " __FUNCSIG__
#else
    // "See T below: " __PRETTY_FUNCTION__

#endif
    );
}
}

namespace _serialization_helper
{
template <typename out_t, typename deserializer_t, typename string_t = default_string_t>
class is_deserializable
{
    template <typename U>
    static auto test(int)
        -> decltype(std::declval<deserializer_t>()(std::declval<basic_value<string_t>>(), std::declval<U&>()), std::true_type());

    template <typename U>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<out_t>(0))::value;
};

struct empty_deserializer
{
    // sample:
    // bool operator()(const json::value&, type_1&) const { return ...; }
    // bool operator()(const json::value&, type_2&) const { return ...; }
    // bool operator()(const json::value&, type_3&) const { return ...; }
};

template <typename T>
void unable_to_deserialize()
{
    static_assert(
        !sizeof(T),
        "Unable to deserialize T. "
#ifdef _MSC_VER
        "See T below: " __FUNCSIG__
#else
    // "See T below: " __PRETTY_FUNCTION__

#endif
    );
}
}

template <
    typename in_t,
    typename serializer_t = _serialization_helper::empty_serializer,
    typename string_t = default_string_t>
basic_value<string_t> serialize(in_t&& in, const serializer_t& serializer = {})
{
    if constexpr (_serialization_helper::is_serializable<in_t, serializer_t>::value) {
        return serializer(std::forward<in_t>(in));
    }
    else if constexpr (
        _utils::is_collection<std::decay_t<in_t>> || _utils::is_fixed_array<std::decay_t<in_t>>) {
        basic_array<string_t> arr;
        for (auto&& elem : in) {
            using elem_t = decltype(elem);

            auto j_elem =
                serialize<elem_t, serializer_t, string_t>(std::forward<elem_t>(elem), serializer);
            arr.emplace_back(std::move(j_elem));
        }
        return arr;
    }
    else if constexpr (_utils::is_map<std::decay_t<in_t>>) {
        basic_object<string_t> obj;
        for (auto&& [key, elem] : in) {
            using key_t = decltype(key);
            using elem_t = decltype(elem);

            auto j_elem =
                serialize<elem_t, serializer_t, string_t>(std::forward<elem_t>(elem), serializer);
            obj.emplace(std::forward<key_t>(key), std::move(j_elem));
        }
        return obj;
    }
    else if constexpr (std::is_constructible_v<basic_value<string_t>, in_t>) {
        return basic_value<string_t>(std::forward<in_t>(in));
    }
    else {
        _serialization_helper::unable_to_serialize<in_t>();
    }
}

template <
    typename out_t,
    typename deserializer_t = _serialization_helper::empty_deserializer,
    typename string_t = default_string_t>
bool deserialize(
    const basic_value<string_t>& in,
    out_t& out,
    const deserializer_t& deserializer = {})
{
    if constexpr (_serialization_helper::is_deserializable<out_t, deserializer_t>::value) {
        return deserializer(in, out);
    }
    else if constexpr (_utils::is_collection<std::decay_t<out_t>>) {
        if (!in.is_array()) {
            return false;
        }
        for (auto&& j_elem : in.as_array()) {
            using elem_t = typename out_t::value_type;
            elem_t elem {};
            if (!deserialize<elem_t, deserializer_t, string_t>(j_elem, elem, deserializer)) {
                return false;
            }
            if constexpr (_as_collection_helper::has_emplace_back<out_t>::value) {
                out.emplace_back(std::move(elem));
            }
            else {
                out.emplace(std::move(elem));
            }
        }
        return true;
    }
    else if constexpr (_utils::is_fixed_array<std::decay_t<out_t>>) {
        if (!in.is_array()) {
            return false;
        }
        auto&& in_as_arr = in.as_array();
        constexpr size_t out_size = _utils::fixed_array_size<out_t>;
        if (in_as_arr.size() != out_size) {
            return false;
        }

        for (size_t i = 0; i < out_size; ++i) {
            auto&& j_elem = in_as_arr.at(i);
            using elem_t = typename out_t::value_type;
            elem_t elem {};
            if (!deserialize<elem_t, deserializer_t, string_t>(j_elem, elem, deserializer)) {
                return false;
            }
            out.at(i) = std::move(elem);
        }
        return true;
    }
    else if constexpr (_utils::is_map<std::decay_t<out_t>>) {
        if (!in.is_object()) {
            return false;
        }
        for (auto&& [key, j_elem] : in.as_object()) {
            using elem_t = typename out_t::mapped_type;
            elem_t elem {};
            if (!deserialize<elem_t, deserializer_t, string_t>(j_elem, elem, deserializer)) {
                return false;
            }
            out.emplace(std::forward<decltype(key)>(key), std::move(elem));
        }
        return true;
    }
    else if constexpr (std::is_constructible_v<out_t, basic_value<string_t>>) {
        out = out_t(in);
        return true;
    }
    else {
        _serialization_helper::unable_to_deserialize<out_t>();
    }
}
} // namespace json
