#pragma once

#include <string>
#include <utility>

namespace adb::stage
{
inline thread_local std::string current;

inline void set(std::string value)
{
    current = std::move(value);
}

inline std::string last()
{
    return current;
}
} // namespace adb::stage
