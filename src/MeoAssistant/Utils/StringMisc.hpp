#pragma once

#include <initializer_list>
#include <iterator>
#include <locale>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "AsstRanges.hpp"
#include "Meta.hpp"

namespace asst::utils
{
    namespace detail
    {
        template <typename StringT = std::string>
        using sv_type = std::basic_string_view<typename std::remove_reference_t<StringT>::value_type,
                                               typename std::remove_reference_t<StringT>::traits_type>;

        template <typename StringT = std::string>
        using sv_pair = std::pair<sv_type<StringT>, sv_type<StringT>>;

    } // namespace detail

    template <typename StringT = std::string>
    inline constexpr void string_replace_all_in_place(StringT& str, detail::sv_type<StringT> from,
                                                      detail::sv_type<StringT> to)
    {
        using size_type = std::string_view::size_type;
        for (size_type pos(0);; pos += to.length()) {
            if ((pos = str.find(from, pos)) == StringT::npos) return;
            str.replace(pos, from.length(), to);
        }
    }

    template <typename StringT = std::string>
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
    inline constexpr auto string_replace_all(StringT&& src, detail::sv_type<StringT> from, detail::sv_type<StringT> to)
    {
        std::basic_string result { std::forward<StringT>(src) };
        string_replace_all_in_place(result, from, to);
        return result;
    }

    template <typename StringT>
    inline constexpr auto string_replace_all(StringT&& src, const detail::sv_pair<StringT>& replace_pair)
    {
        std::basic_string result { std::forward<StringT>(src) };
        string_replace_all_in_place(result, replace_pair);
        return result;
    }

    template <typename StringT>
    inline constexpr auto string_replace_all(StringT&& src,
                                             std::initializer_list<detail::sv_pair<StringT>> replace_pairs)
    {
        std::basic_string result { std::forward<StringT>(src) };
        for (auto&& [from, to] : replace_pairs) {
            string_replace_all_in_place(result, from, to);
        }
        return result;
    }

    template <typename StringT, typename MapT>
    [[deprecated]] inline constexpr auto string_replace_all(StringT&& src, MapT&& replace_pairs)
    {
        std::basic_string result { std::forward<StringT>(src) };
        for (auto&& [from, to] : replace_pairs) {
            string_replace_all_in_place(result, from, to);
        }
        return result;
    }

    template <typename Pred = decltype([](unsigned char c) -> bool { return !std::isspace(c); })>
    inline void string_trim(std::string& s, Pred not_space = Pred {})
    {
        s.erase(ranges::find_if(s | views::reverse, not_space).base(), s.end());
        s.erase(s.begin(), ranges::find_if(s, not_space));
    }

    template <ranges::input_range Rng>
    requires std::convertible_to<ranges::range_value_t<Rng>, char>
    void tolowers(Rng& rng)
    {
        ranges::for_each(rng, [](char& c) -> void { c = static_cast<char>(std::tolower(c)); });
    }

} // namespace asst::utils
