#pragma once

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <locale>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "Meta.hpp"
#include "Ranges.hpp"

namespace asst::utils
{
    namespace detail
    {
        template <typename StringT>
        using sv_type = std::basic_string_view<typename std::remove_reference_t<StringT>::value_type,
                                               typename std::remove_reference_t<StringT>::traits_type>;

        template <typename StringT>
        using sv_pair = std::pair<sv_type<StringT>, sv_type<StringT>>;
    } // namespace detail

    template <typename StringT, typename CharT = ranges::range_value_t<StringT>>
    concept IsSomeKindOfString = std::same_as<CharT, char> || std::same_as<CharT, wchar_t>;

    template <typename StringT>
    requires IsSomeKindOfString<StringT>
    inline constexpr void string_replace_all_in_place(StringT& str, detail::sv_type<StringT> from,
                                                      detail::sv_type<StringT> to)
    {
        for (size_t pos(0);; pos += to.length()) {
            if ((pos = str.find(from, pos)) == StringT::npos) return;
            str.replace(pos, from.length(), to);
        }
    }

    template <typename StringT>
    requires IsSomeKindOfString<StringT>
    inline constexpr void string_replace_all_in_place(StringT& str, const detail::sv_pair<StringT>& replace_pair)
    {
        string_replace_all_in_place(str, replace_pair.first, replace_pair.second);
    }

#ifdef ASST_USE_RANGES_RANGE_V3
    // workaround for P2210R2
    template <ranges::forward_range Rng>
    requires(requires(Rng rng) { std::basic_string_view(std::addressof(*rng.begin()), ranges::distance(rng)); })
    inline auto make_string_view(Rng rng)
    {
        return std::basic_string_view(std::addressof(*rng.begin()), ranges::distance(rng));
    }

    template <std::forward_iterator It, std::sized_sentinel_for<It> End>
    requires(requires(It beg, End end) { std::basic_string_view(std::addressof(*beg), std::distance(beg, end)); })
    inline auto make_string_view(It beg, End end)
    {
        return std::basic_string_view(std::addressof(*beg), std::distance(beg, end));
    }
#else
    template <ranges::contiguous_range Rng>
    inline auto make_string_view(Rng rng)
    {
        return std::basic_string_view(rng.begin(), rng.end());
    }

    template <std::contiguous_iterator It, std::sized_sentinel_for<It> End>
    requires(requires(It beg, End end) { std::basic_string_view(beg, end); })
    inline auto make_string_view(It beg, End end)
    {
        return std::basic_string_view(beg, end);
    }
#endif

    template <typename StringT>
    requires IsSomeKindOfString<StringT>
    [[nodiscard]] inline constexpr auto string_replace_all(StringT&& src, detail::sv_type<StringT> from,
                                                           detail::sv_type<StringT> to)
    {
        std::decay_t<StringT> result = std::forward<StringT>(src);
        string_replace_all_in_place(result, from, to);
        return result;
    }

    template <typename StringT>
    requires IsSomeKindOfString<StringT>
    [[nodiscard]] inline constexpr auto string_replace_all(StringT&& src, const detail::sv_pair<StringT>& replace_pair)
    {
        std::decay_t<StringT> result = std::forward<StringT>(src);
        string_replace_all_in_place(result, replace_pair);
        return result;
    }

    template <typename StringT>
    requires IsSomeKindOfString<StringT>
    [[nodiscard]] inline constexpr auto string_replace_all(
        StringT&& str, std::initializer_list<detail::sv_pair<StringT>> replace_pairs)
    {
        std::decay_t<StringT> result = std::forward<StringT>(str);
        for (auto&& [from, to] : replace_pairs) {
            string_replace_all_in_place(result, from, to);
        }
        return result;
    }

    template <typename StringT, typename CharT = ranges::range_value_t<StringT>>
    requires IsSomeKindOfString<StringT>
    inline void string_trim(
        StringT& str, const std::function<bool(CharT)>& not_space = [](CharT ch) -> bool { return !std::isspace(ch); })
    {
        str.erase(ranges::find_if(str | views::reverse, not_space).base(), str.end());
        str.erase(str.begin(), ranges::find_if(str, not_space));
    }

    template <typename StringT, typename CharT = ranges::range_value_t<StringT>>
    requires IsSomeKindOfString<StringT>
    inline void tolowers(StringT& str)
    {
        ranges::for_each(str, [](CharT& ch) -> void { ch = static_cast<CharT>(std::tolower(ch)); });
    }

    template <typename StringT, typename CharT = ranges::range_value_t<StringT>>
    requires IsSomeKindOfString<StringT>
    inline void touppers(StringT& str)
    {
        ranges::for_each(str, [](CharT& ch) -> void { ch = static_cast<CharT>(std::toupper(ch)); });
    }
} // namespace asst::utils
