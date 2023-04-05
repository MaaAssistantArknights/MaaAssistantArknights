#pragma once

#include <cstdio>
#include <string>

#ifdef _WIN32
#include "Platform/SafeWindows.h"
#else
#include <ctime>
#include <fcntl.h>
#include <sys/time.h>
#endif

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
        auto offset = strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", tm_info);
        sprintf(buff + offset, ".%03ld", static_cast<long int>(tv.tv_usec / 1000));
#endif // END _WIN32
        return buff;
    }

    inline std::string get_time_filestem()
    {
        std::string stem = utils::get_format_time();
        string_replace_all_in_place(stem, { { ":", "-" }, { " ", "_" }, { ".", "-" } });
        return stem;
    }
} // namespace asst::utils
