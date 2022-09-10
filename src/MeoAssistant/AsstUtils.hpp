#pragma once

#include "AsstConf.h"
#include "AsstRanges.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include "SafeWindows.h"
#else
#include <ctime>
#include <fcntl.h>
#include <memory.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#ifndef _MSC_VER
#include <cxxabi.h>
#endif

// delete instantiation of template with message, when static_assert(false, Message) does not work
#define ASST_STATIC_ASSERT_FALSE(Message, ...) static_assert(::asst::utils::always_false<__VA_ARGS__>, Message);

namespace asst::utils
{
    using os_string = std::filesystem::path::string_type;
    template <typename... Unused>
    constexpr bool always_false = false;

    template <typename char_t = char>
    using pair_of_string_view = std::pair<std::basic_string_view<char_t>, std::basic_string_view<char_t>>;

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

    // 延迟求值的 string_split，分隔符可以为字符或字符串
    template <typename char_t>
    class string_split_view : public ranges::view_interface<string_split_view<char_t>>
    {
    private:
        using base_string_view = std::basic_string_view<char_t>;
        using base_string = std::basic_string<char_t>;
        using string_iter = typename base_string_view::const_iterator;
        base_string_view _rng {};
        base_string_view _pat {};
        base_string_view _nxt {};

        class _iterator
        {
        private:
            string_split_view* _pre = nullptr;
            string_iter _cur = {};
            base_string_view _nxt = {};
            bool _trailing_empty = false;

        public:
            using iterator_concept = std::forward_iterator_tag;
            using iterator_category = std::input_iterator_tag;
            using value_type = base_string_view;
            using difference_type = typename base_string_view::difference_type;

            _iterator() = default;

            constexpr _iterator(string_split_view& prev, string_iter curr, base_string_view next) noexcept
                : _pre { std::addressof(prev) }, _cur { std::move(curr) }, _nxt { std::move(next) }
            {}

            [[nodiscard]] constexpr string_iter base() const noexcept { return _cur; }

            [[nodiscard]] constexpr value_type operator*() const noexcept { return { _cur, _nxt.cbegin() }; }

            constexpr _iterator& operator++()
            {
                auto _end = _pre->_rng.cend();

                if (_cur = _nxt.cbegin(); _cur == _end) {
                    _trailing_empty = false;
                    return *this;
                }

                if (_cur = _nxt.cend(); _cur == _end) {
                    _trailing_empty = true;
                    _nxt = { _cur, _cur };
                    return *this;
                }

                if (size_t _len = _pre->_pat.length(); !_len) {
                    auto _beg = _cur + 1;
                    _nxt = { std::move(_beg), std::move(_beg) };
                }
                else if (const auto _pos = base_string_view(_cur, _end).find(_pre->_pat);
                         _pos == base_string_view::npos) {
                    _nxt = { std::move(_end), std::move(_end) };
                }
                else {
                    auto _beg = _cur + _pos;
                    _nxt = { std::move(_beg), _beg + _pre->_pat.length() };
                }

                return *this;
            }

            constexpr _iterator operator++(int)
            {
                auto _tmp = *this;
                ++*this;
                return _tmp;
            }

            friend constexpr bool operator==(const _iterator& _lhs, const _iterator& _rhs) noexcept
            {
                return _lhs._cur == _rhs._cur && _lhs._trailing_empty == _rhs._trailing_empty;
            }
        };

    public:
        string_split_view() = default;

        template <typename rng_t, typename pat_t>
        requires std::convertible_to<pat_t, base_string_view>
        constexpr string_split_view(rng_t&& rang, pat_t&& patt) noexcept
            : _rng(std::forward<rng_t>(rang)), _pat(std::forward<pat_t>(patt))
        {}

        template <typename rng_t>
        constexpr string_split_view(rng_t&& rang, char_t&& elem)
        {
            // 摆烂辣！解决不了临时变量导致 _pat 悬垂的问题
            ASST_STATIC_ASSERT_FALSE("please use `views::split` instead ^_^", rng_t);
        }

        template <typename rng_t>
        constexpr string_split_view(rng_t&& rang, const char_t& elem) noexcept
            : _rng(std::forward<rng_t>(rang)), _pat(&elem, 1)
        {}

        [[nodiscard]] constexpr const base_string_view& base() const noexcept { return _rng; }
        [[nodiscard]] constexpr base_string_view&& base() noexcept { return std::move(_rng); }

        [[nodiscard]] constexpr auto begin()
        {
            const auto _beg = _rng.cbegin(), _end = _rng.cend();

            if (_nxt.empty()) {
                if (size_t _len = _pat.length(); !_len) {
                    auto _cur = _beg + 1;
                    _nxt = { std::move(_cur), std::move(_cur) };
                }
                else if (const auto _pos = _rng.find(_pat); _pos == base_string_view::npos) {
                    _nxt = { std::move(_end), std::move(_end) };
                }
                else {
                    auto _cur = _beg + _pos;
                    _nxt = { std::move(_cur), _cur + _pat.length() };
                }
            }
            return _iterator { *this, _beg, _nxt };
        }

        [[nodiscard]] constexpr auto end() { return _iterator { *this, _rng.cend(), {} }; }
    };

    template <ranges::range str_t, class pat_t>
    string_split_view(str_t, pat_t) -> string_split_view<typename str_t::value_type>;

    template <class str_t, class pat_t>
    string_split_view(str_t*, pat_t) -> string_split_view<typename std::remove_cv<str_t>::type>;

    struct _string_split_fn
    {
        template <ranges::range str_t, class pat_t>
        [[nodiscard]] constexpr auto operator()(str_t&& _str, pat_t&& _pat) const
            noexcept(noexcept(string_split_view(std::forward<str_t>(_str), std::forward<pat_t>(_pat))))
        requires requires { string_split_view(static_cast<str_t&&>(_str), static_cast<pat_t&&>(_pat)); }
        {
            return string_split_view(std::forward<str_t>(_str), std::forward<pat_t>(_pat));
        }
    };

    inline constexpr _string_split_fn string_split;

    // // 以低内存开销拆分一个字符串；注意当 src 析构后，返回值将失效
    // template <typename str_t, typename char_t = typename str_t::value_type, typename delimiter_t>
    // requires std::convertible_to<str_t, std::basic_string_view<char_t>> &&
    //          std::convertible_to<delimiter_t, std::basic_string_view<char_t>>
    // inline std::vector<std::basic_string_view<char_t>> string_split(const str_t& src, delimiter_t delimiter,
    //                                                                 size_t split_count = -1)
    // {
    //     std::basic_string_view<char_t> delimiter_view = delimiter;
    //     std::basic_string_view<char_t> str = src;
    //     typename std::basic_string<char_t>::size_type pos1 = 0;
    //     typename std::basic_string<char_t>::size_type pos2 = str.find(delimiter_view);
    //     std::vector<std::basic_string_view<char_t>> result;

    //     while (split_count-- && pos2 != str.npos) {
    //         result.emplace_back(str.substr(pos1, pos2 - pos1));

    //         pos1 = pos2 + delimiter_view.length();
    //         pos2 = str.find(delimiter_view, pos1);
    //     }
    //     if (pos1 != str.length()) result.emplace_back(str.substr(pos1));

    //     return result;
    // }

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
        sprintf(buff, "%s.%03ld", buff, tv.tv_usec / 1000);
#endif // END _WIN32
        return buff;
    }

#ifdef _WIN32
    static_assert(std::same_as<os_string, std::wstring>);
    os_string to_osstring(const std::string& utf8_str);
    std::string from_osstring(const os_string& os_str);
#else
    static_assert(std::same_as<os_string, std::string>);
    inline os_string to_osstring(const std::string& utf8_str)
    {
        return utf8_str;
    }
    inline std::string from_osstring(const os_string& os_str)
    {
        return os_str;
    }
#endif

    inline std::filesystem::path path(const os_string& os_str)
    {
        return std::filesystem::path(os_str);
    }

#ifdef _WIN32
    // Allow construct a path from utf8-string in win32
    inline std::filesystem::path path(const std::string& utf8_str)
    {
        return std::filesystem::path(to_osstring(utf8_str));
    }
#endif

#ifdef _WIN32

    std::string path_to_crt_string(const std::filesystem::path& path);

    std::string path_to_ansi_string(const std::filesystem::path& path);

    inline std::string path_to_utf8_string(const std::filesystem::path& path)
    {
        return from_osstring(path.native());
    }

    inline std::string path_to_crt_string(const std::string& utf8_path)
    {
        return path_to_crt_string(path(utf8_path));
    }

    inline std::string path_to_ansi_string(const std::string& utf8_path)
    {
        return path_to_crt_string(path(utf8_path));
    }

#else

    inline std::string path_to_utf8_string(const std::filesystem::path& path)
    {
        return path.native();
    }

    inline std::string path_to_ansi_string(const std::filesystem::path& path)
    {
        return path.native();
    }

    inline std::string path_to_crt_string(const std::filesystem::path& path)
    {
        return path.native();
    }

#endif

    template <typename _ = void>
    inline std::string ansi_to_utf8(std::string_view ansi_str)
    {
#ifdef _WIN32
        const char* src_str = ansi_str.data();
        int len = MultiByteToWideChar(CP_ACP, 0, src_str, -1, nullptr, 0);
        const std::size_t wstr_length = static_cast<std::size_t>(len) + 1U;
        auto wstr = new wchar_t[wstr_length];
        memset(wstr, 0, sizeof(wstr[0]) * wstr_length);
        MultiByteToWideChar(CP_ACP, 0, src_str, -1, wstr, len);

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
        int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, nullptr, 0);
        const std::size_t wsz_ansi_length = static_cast<std::size_t>(len) + 1U;
        auto wsz_ansi = new wchar_t[wsz_ansi_length];
        memset(wsz_ansi, 0, sizeof(wsz_ansi[0]) * wsz_ansi_length);
        MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wsz_ansi, len);

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

        static constexpr char _Bom[] = {
            static_cast<char>(0xEF),
            static_cast<char>(0xBB),
            static_cast<char>(0xBF),
        };

        if (str.starts_with(_Bom)) {
            str.assign(str.begin() + 3, str.end());
        }
        return str;
    }

    std::string callcmd(const std::string& cmdline);

    inline std::string demangle(const char* name_from_typeid)
    {
#ifndef _MSC_VER
        int status = 0;
        std::size_t size = 0;
        char* p = abi::__cxa_demangle(name_from_typeid, NULL, &size, &status);
        if (!p) return name_from_typeid;
        std::string result(p);
        std::free(p);
        return result;
#else
        std::string_view temp(name_from_typeid);
        if (temp.substr(0, 6) == "class ") return std::string(temp.substr(6));
        if (temp.substr(0, 7) == "struct ") return std::string(temp.substr(7));
        if (temp.substr(0, 5) == "enum ") return std::string(temp.substr(5));
        return std::string(temp);
#endif
    }

    namespace path_literals
    {
        inline std::filesystem::path operator"" _p(const char* utf8_str, size_t len)
        {
            // 日后再优化（
            return asst::utils::path(std::string(std::string_view(utf8_str, len)));
        }
    }
} // namespace asst::utils
