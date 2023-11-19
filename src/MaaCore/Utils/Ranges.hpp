#pragma once

#include "Common/AsstConf.h"

#ifdef ASST_USE_RANGES_RANGE_V3

#include <range/v3/all.hpp>

namespace asst
{
    namespace ranges = ::ranges;
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
