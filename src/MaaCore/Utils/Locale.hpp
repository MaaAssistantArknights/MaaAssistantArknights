#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include "Meta.hpp"

#ifdef _WIN32
#include <locale.h>
#include <io.h>
#include "Platform/SafeWindows.h"
#endif

namespace asst::utils
{
    template <typename _ = void>
    inline std::string ansi_to_utf8(std::string_view ansi_str)
    {
#ifdef _WIN32
        const char* src_str = ansi_str.data();
        const int byte_len = static_cast<int>(ansi_str.length() * sizeof(char));
        int len = MultiByteToWideChar(CP_ACP, 0, src_str, byte_len, nullptr, 0);
        const std::size_t wstr_length = static_cast<std::size_t>(len) + 1U;
        auto wstr = new wchar_t[wstr_length];
        memset(wstr, 0, sizeof(wstr[0]) * wstr_length);
        MultiByteToWideChar(CP_ACP, 0, src_str, byte_len, wstr, len);

        len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
        const std::size_t str_length = static_cast<std::size_t>(len) + 1;
        auto str = new char[str_length];
        memset(str, 0, sizeof(str[0]) * str_length);
        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, nullptr, nullptr);
        std::string strTemp = str;

        delete[] wstr;
        wstr = nullptr;
        delete[] str;
        str = nullptr;

        return strTemp;
#else // Don't fucking use gbk in linux!
        ASST_STATIC_ASSERT_FALSE("Workaround for windows, not implemented in other OS yet.", _);
        return std::string(ansi_str);
#endif
    }

    template <typename _ = void>
    inline std::string utf8_to_ansi(std::string_view utf8_str)
    {
#ifdef _WIN32
        const char* src_str = utf8_str.data();
        const int byte_len = static_cast<int>(utf8_str.length() * sizeof(char));
        int len = MultiByteToWideChar(CP_UTF8, 0, src_str, byte_len, nullptr, 0);
        const std::size_t wsz_ansi_length = static_cast<std::size_t>(len) + 1U;
        auto wsz_ansi = new wchar_t[wsz_ansi_length];
        memset(wsz_ansi, 0, sizeof(wsz_ansi[0]) * wsz_ansi_length);
        MultiByteToWideChar(CP_UTF8, 0, src_str, byte_len, wsz_ansi, len);

        len = WideCharToMultiByte(CP_ACP, 0, wsz_ansi, -1, nullptr, 0, nullptr, nullptr);
        const std::size_t sz_ansi_length = static_cast<std::size_t>(len) + 1;
        auto sz_ansi = new char[sz_ansi_length];
        memset(sz_ansi, 0, sizeof(sz_ansi[0]) * sz_ansi_length);
        WideCharToMultiByte(CP_ACP, 0, wsz_ansi, -1, sz_ansi, len, nullptr, nullptr);
        std::string strTemp(sz_ansi);

        delete[] wsz_ansi;
        wsz_ansi = nullptr;
        delete[] sz_ansi;
        sz_ansi = nullptr;

        return strTemp;
#else // Don't fucking use gbk in linux!
        ASST_STATIC_ASSERT_FALSE("Workaround for windows, not implemented in other OS yet.", _);
        return std::string(utf8_str);
#endif
    }

    template <typename _ = void>
    inline std::string utf8_to_unicode_escape(std::string_view utf8_str)
    {
#ifdef _WIN32
        const char* src_str = utf8_str.data();
        int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, nullptr, 0);
        const std::size_t wstr_length = static_cast<std::size_t>(len) + 1U;
        auto wstr = new wchar_t[wstr_length];
        memset(wstr, 0, sizeof(wstr[0]) * wstr_length);
        MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wstr, len);

        std::string unicode_escape_str = {};
        constinit static char hexcode[] = "0123456789abcdef";
        for (const wchar_t* pchr = wstr; *pchr; ++pchr) {
            const wchar_t& chr = *pchr;
            if (chr > 255) {
                unicode_escape_str += "\\u";
                unicode_escape_str.push_back(hexcode[chr >> 12]);
                unicode_escape_str.push_back(hexcode[(chr >> 8) & 15]);
                unicode_escape_str.push_back(hexcode[(chr >> 4) & 15]);
                unicode_escape_str.push_back(hexcode[chr & 15]);
            }
            else {
                unicode_escape_str.push_back(chr & 255);
            }
        }

        delete[] wstr;
        wstr = nullptr;

        return unicode_escape_str;
#else
        ASST_STATIC_ASSERT_FALSE("Workaround for windows, not implemented in other OS yet.", _);
        return std::string(utf8_str);
#endif
    }

    inline std::string load_file_without_bom(const std::filesystem::path& path)
    {
        std::ifstream ifs(path, std::ios::in);
        if (!ifs.is_open()) {
            return {};
        }
        std::stringstream iss;
        iss << ifs.rdbuf();
        ifs.close();
        std::string str = iss.str();

        if (str.starts_with("\xEF\xBB\xBF")) {
            str.assign(str.begin() + 3, str.end());
        }
        return str;
    }

    class utf8_scope
    {
#ifdef _WIN32
        bool active;
        int thread_mode;
        std::string old_locale;

        static bool is_console_ostream(std::ostream& ofs)
        {
            if (&ofs == &std::cout) {
                return _isatty(_fileno(stdout));
            }
            if (&ofs == &std::cerr) {
                return _isatty(_fileno(stderr));
            }
            return false;
        }

    public:
        utf8_scope(std::nullptr_t)
        {
            active = false;
            thread_mode = 0;
        }

        ~utf8_scope()
        {
            if (!active) return;
            if (!old_locale.empty()) {
                setlocale(LC_ALL, old_locale.c_str());
            }
            _configthreadlocale(_DISABLE_PER_THREAD_LOCALE);
        }

        utf8_scope(std::ostream& ofs)
        {
            if (is_console_ostream(ofs)) {
                active = true;
                thread_mode = _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
                auto old_locale_ptr = setlocale(LC_ALL, ".UTF-8");
                if (old_locale_ptr) {
                    old_locale = old_locale_ptr;
                }
            }
            else {
                active = false;
            }
        }
#else
    public:
        utf8_scope(std::ostream&)
        {
        }
#endif
    };
} // namespace asst::utils
