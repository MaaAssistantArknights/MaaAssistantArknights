#pragma once

#include "AsstConf.h"

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

#else

#error

#endif