#pragma once

#include <cstdint>

namespace arps {

#ifdef _WIN32
using ArpsSocket = std::uintptr_t;
constexpr ArpsSocket kInvalidArpsSocket =
        static_cast<ArpsSocket>(~static_cast<std::uintptr_t>(0));
#else
using ArpsSocket = int;
constexpr ArpsSocket kInvalidArpsSocket = -1;
#endif

}  // namespace arps
