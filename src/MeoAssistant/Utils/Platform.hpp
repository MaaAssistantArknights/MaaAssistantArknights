#pragma once

#include "Platform/Platform.h"

namespace asst::utils
{
    using os_string = platform::os_string;

#ifdef _WIN32
    static_assert(std::same_as<os_string, std::wstring>);
#else
    static_assert(std::same_as<os_string, std::string>);
#endif

    using platform::from_osstring;
    using platform::path;
    using platform::path_to_ansi_string;
    using platform::path_to_crt_string;
    using platform::path_to_utf8_string;
    using platform::to_osstring;

    using platform::call_command;

    namespace path_literals
    {
        inline std::filesystem::path operator"" _p(const char* utf8_str, size_t len)
        {
            // 日后再优化（
            return asst::utils::path(std::string(std::string_view(utf8_str, len)));
        }
    }
} // namespace asst::utils
