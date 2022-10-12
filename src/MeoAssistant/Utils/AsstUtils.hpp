#pragma once

#include "AsstConf.h"
#include "AsstRanges.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "Platform/AsstPlatform.h"

#ifdef _WIN32
#include "Platform/SafeWindows.h"
#else
#include <ctime>
#include <fcntl.h>
#include <sys/time.h>
#endif

#include "Locale.hpp"
#include "Meta.hpp"
#include "StringMisc.hpp"

namespace asst::utils
{

    inline std::string get_format_time()
    {
        char buff[128] = { 0 };
#ifdef _WIN32
        SYSTEMTIME curtime;
        GetLocalTime(&curtime);
#ifdef _MSC_VER
        sprintf_s(buff, sizeof(buff),
#else  // ! _MSC_VER
        sprintf(buff,
#endif // END _MSC_VER
                  "%04d-%02d-%02d %02d:%02d:%02d.%03d", curtime.wYear, curtime.wMonth, curtime.wDay, curtime.wHour,
                  curtime.wMinute, curtime.wSecond, curtime.wMilliseconds);

#else  // ! _WIN32
        struct timeval tv = {};
        gettimeofday(&tv, nullptr);
        time_t nowtime = tv.tv_sec;
        struct tm* tm_info = localtime(&nowtime);
        strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", tm_info);
        sprintf(buff, "%s.%03ld", buff, static_cast<long int>(tv.tv_usec / 1000));
#endif // END _WIN32
        return buff;
    }

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

    using platform::callcmd;

    namespace path_literals
    {
        inline std::filesystem::path operator"" _p(const char* utf8_str, size_t len)
        {
            // 日后再优化（
            return asst::utils::path(std::string(std::string_view(utf8_str, len)));
        }
    }

} // namespace asst::utils
