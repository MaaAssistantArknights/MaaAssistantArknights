#pragma once

#include "Common/AsstConf.h"

#ifdef ASST_USE_RANGES_RANGE_V3

#include <range/v3/all.hpp>

namespace asst
{
    namespace ranges
    {
        using namespace ::ranges;

        // return type of ::ranges::remove_if is different from which of std::remove_if
        struct remove_if_fn
        {
            template <permutable I, sentinel_for<I> S, typename C, typename P = identity>
            constexpr subrange<I> operator()(I first, S last, C pred, P proj = P {}) const
            {
                return { ::ranges::remove_if(std::move(first), last, std::move(pred), std::move(proj)), last };
            }

            template <forward_range Rng, typename C, typename P = identity>
            constexpr borrowed_subrange_t<Rng> operator()(Rng&& rng, C pred, P proj = P {}) const
            {
                return (*this)(begin(rng), end(rng), std::move(pred), std::move(proj));
            }
        };

        // this one will be found before ::ranges::remove_if
        inline constexpr remove_if_fn remove_if {};
    }

    namespace views = ::ranges::views;
}

#elif defined(ASST_USE_RANGES_STL)

#include <ranges>

namespace asst
{
    namespace ranges = std::ranges;
    namespace views = std::views;
}

#elif defined(ASST_USE_RANGES_BOOST)

#error "Not implemented"

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

namespace asst
{
    namespace ranges = boost::range;
    namespace views = boost::adaptors;
}

#else

#error

#endif
