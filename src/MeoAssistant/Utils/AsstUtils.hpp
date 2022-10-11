#pragma once

#include "AsstConf.h"
#include "AsstRanges.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
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

#include "Meta.hpp"
#include "SingletonHolder.hpp"

namespace asst::utils
{
    template <typename char_t = char>
    using pair_of_string_view = std::pair<std::basic_string_view<char_t>, std::basic_string_view<char_t>>;

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

    template <typename char_t>
    inline void _string_replace_all(std::basic_string<char_t>& str, std::basic_string_view<char_t> old_value,
                                    std::basic_string_view<char_t> new_value)
    {
        for (typename std::basic_string<char_t>::size_type pos(0); pos != str.npos; pos += new_value.length()) {
            if ((pos = str.find(old_value, pos)) != str.npos)
                str.replace(pos, old_value.length(), new_value);
            else
                break;
        }
    }

    template <typename char_t, typename old_value_t, typename new_value_t>
    requires std::convertible_to<old_value_t, std::basic_string_view<char_t>> &&
             std::convertible_to<new_value_t, std::basic_string_view<char_t>>
    inline void _string_replace_all(std::basic_string<char_t>& str, old_value_t&& old_value, new_value_t&& new_value)
    {
        _string_replace_all(str, { old_value }, { new_value });
    }

    template <typename char_t>
    inline void _string_replace_all(std::basic_string<char_t>& str, const pair_of_string_view<char_t>& replace_pair)
    {
        _string_replace_all(str, replace_pair.first, replace_pair.second);
    }

    template <typename char_t, typename old_value_t, typename new_value_t>
    requires std::convertible_to<old_value_t, std::basic_string_view<char_t>> &&
             std::convertible_to<new_value_t, std::basic_string_view<char_t>>
    inline std::basic_string<char_t> string_replace_all(const std::basic_string<char_t>& src, old_value_t&& old_value,
                                                        new_value_t&& new_value)
    {
        std::basic_string<char_t> str = src;
        _string_replace_all(str, { old_value }, { new_value });
        return str;
    }

    template <typename char_t>
    inline std::basic_string<char_t> string_replace_all(const std::basic_string<char_t>& src,
                                                        const pair_of_string_view<char_t>& replace_pair)
    {
        std::basic_string<char_t> str = src;
        _string_replace_all(str, replace_pair);
        return str;
    }

    template <typename char_t>
    inline std::basic_string<char_t> string_replace_all(
        const std::basic_string<char_t>& src, std::initializer_list<pair_of_string_view<char_t>>&& replace_pairs)
    {
        std::basic_string<char_t> str = src;
        for (const auto& [old_value, new_value] : replace_pairs) {
            _string_replace_all(str, old_value, new_value);
        }
        return str;
    }

    template <typename char_t, typename map_t>
    requires std::derived_from<typename map_t::value_type::first_type, std::basic_string<char_t>> &&
             std::derived_from<typename map_t::value_type::second_type, std::basic_string<char_t>>
    [[deprecated]] inline std::basic_string<char_t> string_replace_all(const std::basic_string<char_t>& src,
                                                                       const map_t& replace_pairs)
    {
        std::basic_string<char_t> str = src;
        for (const auto& [old_value, new_value] : replace_pairs) {
            _string_replace_all(str, old_value, new_value);
        }
        return str;
    }

    inline void string_trim(std::string& s)
    {
        auto not_space = [](unsigned char c) { return !std::isspace(c); };
        s.erase(ranges::find_if(s | views::reverse, not_space).base(), s.end());
        s.erase(s.begin(), ranges::find_if(s, not_space));
    }

    template <ranges::input_range Rng>
    requires std::convertible_to<ranges::range_value_t<Rng>, char>
    void tolowers(Rng& rng)
    {
        ranges::transform(rng, rng.begin(), [](char c) -> char { return static_cast<char>(std::tolower(c)); });
    }

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

    template <typename RetTy, typename ArgType>
    constexpr inline RetTy make_rect(const ArgType& rect)
    {
        return RetTy { rect.x, rect.y, rect.width, rect.height };
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

    using platform::callcmd;

    namespace path_literals
    {
        inline std::filesystem::path operator"" _p(const char* utf8_str, size_t len)
        {
            // 日后再优化（
            return asst::utils::path(std::string(std::string_view(utf8_str, len)));
        }
    }

    class UserDir : public SingletonHolder<UserDir>
    {
    public:
        bool empty() const noexcept { return user_dir_.empty(); }
        const std::filesystem::path& get() const noexcept { return user_dir_; }
        bool set(const char* dir)
        {
            auto temp = path(dir);
            if (!std::filesystem::exists(temp) || !std::filesystem::is_directory(temp)) {
                return false;
            }
            user_dir_ = temp;
            return true;
        }

    private:
        std::filesystem::path user_dir_;
    };
} // namespace asst::utils
