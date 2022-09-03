#pragma once

#include "AsstConf.h"
#include "AsstRanges.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <Windows.h>
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

namespace asst::utils
{
//  delete instantiation of template with message, when static_assert(false, Message) does not work
#define ASST_STATIC_ASSERT_FALSE(Message, ...) \
    static_assert(::asst::utils::integral_constant_at_template_instantiation<bool, false, __VA_ARGS__>::value, Message)
    template <typename T, T Val, typename... Unused>
    struct integral_constant_at_template_instantiation : std::integral_constant<T, Val>
    {};

    template <typename dst_t, typename src_t>
    requires ranges::range<src_t> && ranges::range<dst_t>
    dst_t view_cast(src_t&& src)
    {
        return dst_t(src.begin(), src.end());
    }

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
        base_string_view _Range {};
        base_string_view _Pattern {};
        ranges::_Non_propagating_cache<base_string_view> _Next {};
        using string_iter = base_string_view::const_iterator;

        class _Iterator
        {
        private:
            string_split_view* _Parent = nullptr;
            string_iter _Current = {};
            base_string_view _Next = {};
            bool _Trailing_empty = false;

        public:
            using iterator_concept = std::forward_iterator_tag;
            using iterator_category = std::input_iterator_tag;
            using value_type = base_string_view;
            using difference_type = base_string_view::difference_type;

            _Iterator() = default;

            constexpr _Iterator(string_split_view& _Parent_, string_iter _Current_, base_string_view _Next_) noexcept
                : _Parent { std::addressof(_Parent_) }, _Current { std::move(_Current_) }, _Next { std::move(_Next_) }
            {}

            [[nodiscard]] constexpr string_iter base() const noexcept { return _Current; }

            [[nodiscard]] constexpr value_type operator*() const noexcept { return { _Current, _Next.cbegin() }; }

            constexpr _Iterator& operator++()
            {
                const auto _Last = _Parent->_Range.cend();
                _Current = _Next.cbegin();
                if (_Current == _Last) {
                    _Trailing_empty = false;
                    return *this;
                }

                _Current = _Next.cend();
                if (_Current == _Last) {
                    _Trailing_empty = true;
                    _Next = { _Current, _Current };
                    return *this;
                }

                if (const auto _pos = base_string_view(_Current, _Last).find(_Parent->_Pattern);
                    _pos == base_string_view::npos) {
                    _Next = { std::move(_Last), std::move(_Last) };
                }
                else {
                    _Next = { _Current + _pos, _Current + _pos + _Parent->_Pattern.length() };
                }

                return *this;
            }

            constexpr _Iterator operator++(int)
            {
                auto _Tmp = *this;
                ++*this;
                return _Tmp;
            }

            friend constexpr bool operator==(const _Iterator& _Left, const _Iterator& _Right) noexcept
            {
                return _Left._Current == _Right._Current && _Left._Trailing_empty == _Right._Trailing_empty;
            }
        };

        constexpr base_string_view _Find_next(string_iter _It)
        {
            const auto _Last = _Range.cend();

            if (const auto _pos = base_string_view(_It, _Last).find(_Pattern); _pos == base_string_view::npos) {
                return { std::move(_Last), std::move(_Last) };
            }
            else {
                return { _It + _pos, _It + _pos + _Pattern.length() };
            }
        }

    public:
        string_split_view() = default;

        template <typename _Tp1, typename _Tp2>
        requires std::convertible_to<_Tp2, base_string_view>
        constexpr string_split_view(_Tp1&& _Range_, _Tp2&& _Pattern_) noexcept
            : _Range(std::forward<_Tp1>(_Range_)), _Pattern(std::forward<_Tp2>(_Pattern_))
        {}

        template <typename _Tp1>
        constexpr string_split_view(_Tp1&& _Range_, const char_t& _Elem) noexcept
            : _Range(std::forward<_Tp1>(_Range_)), _Pattern(&_Elem, 1)
        {}

        [[nodiscard]] constexpr const base_string_view& base() const noexcept { return _Range; }
        [[nodiscard]] constexpr base_string_view&& base() noexcept { return std::move(_Range); }

        [[nodiscard]] constexpr auto begin()
        {
            auto _First = _Range.cbegin();
            if (!_Next) {
                _Next._Emplace(_Find_next(_First));
            }
            return _Iterator { *this, _First, *_Next };
        }

        [[nodiscard]] constexpr auto end() { return _Iterator { *this, _Range.cend(), {} }; }
    };

    template <ranges::range str_t, class pat_t>
    string_split_view(str_t, pat_t) -> string_split_view<typename str_t::value_type>;

    template <class str_t, class pat_t>
    string_split_view(str_t*, pat_t) -> string_split_view<typename std::remove_cv<str_t>::type>;

    struct _string_split_fn
    {
        template <ranges::range str_t, class pat_t>
        [[nodiscard]] constexpr auto operator()(str_t&& str, pat_t&& _Pattern) const
            noexcept(noexcept(string_split_view(std::forward<str_t>(str), std::forward<pat_t>(_Pattern))))
        requires requires { string_split_view(static_cast<str_t&&>(str), static_cast<pat_t&&>(_Pattern)); }
        {
            return string_split_view(std::forward<str_t>(str), std::forward<pat_t>(_Pattern));
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

    template <typename _ = void>
    inline std::string ansi_to_utf8(const std::string& ansi_str)
    {
#ifdef _WIN32
        const char* src_str = ansi_str.c_str();
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
        return ansi_str;
#endif
    }

    template <typename _ = void>
    inline std::string utf8_to_ansi(const std::string& utf8_str)
    {
#ifdef _WIN32
        const char* src_str = utf8_str.c_str();
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
        return utf8_str;
#endif
    }

    template <typename _ = void>
    inline std::string utf8_to_unicode_escape(const std::string& utf8_str)
    {
#ifdef _WIN32
        const char* src_str = utf8_str.c_str();
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
        return utf8_str;
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

    inline std::string callcmd(const std::string& cmdline)
    {
        constexpr int PipeBuffSize = 4096;
        std::string pipe_str;
        auto pipe_buffer = std::make_unique<char[]>(PipeBuffSize);

#ifdef _WIN32
        ASST_AUTO_DEDUCED_ZERO_INIT_START
        SECURITY_ATTRIBUTES pipe_sec_attr = { 0 };
        ASST_AUTO_DEDUCED_ZERO_INIT_END
        pipe_sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
        pipe_sec_attr.lpSecurityDescriptor = nullptr;
        pipe_sec_attr.bInheritHandle = TRUE;
        HANDLE pipe_read = nullptr;
        HANDLE pipe_child_write = nullptr;
        CreatePipe(&pipe_read, &pipe_child_write, &pipe_sec_attr, PipeBuffSize);

        ASST_AUTO_DEDUCED_ZERO_INIT_START
        STARTUPINFOA si = { 0 };
        ASST_AUTO_DEDUCED_ZERO_INIT_END
        si.cb = sizeof(STARTUPINFO);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.wShowWindow = SW_HIDE;
        si.hStdOutput = pipe_child_write;
        si.hStdError = pipe_child_write;

        ASST_AUTO_DEDUCED_ZERO_INIT_START
        PROCESS_INFORMATION pi = { nullptr };
        ASST_AUTO_DEDUCED_ZERO_INIT_END

        BOOL p_ret = CreateProcessA(nullptr, const_cast<LPSTR>(cmdline.c_str()), nullptr, nullptr, TRUE,
                                    CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        if (p_ret) {
            DWORD peek_num = 0;
            DWORD read_num = 0;
            do {
                while (PeekNamedPipe(pipe_read, nullptr, 0, nullptr, &peek_num, nullptr) && peek_num > 0) {
                    if (ReadFile(pipe_read, pipe_buffer.get(), peek_num, &read_num, nullptr)) {
                        pipe_str.append(pipe_buffer.get(), pipe_buffer.get() + read_num);
                    }
                }
            } while (WaitForSingleObject(pi.hProcess, 0) == WAIT_TIMEOUT);

            DWORD exit_ret = 255;
            GetExitCodeProcess(pi.hProcess, &exit_ret);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }

        CloseHandle(pipe_read);
        CloseHandle(pipe_child_write);

#else
        static constexpr int PIPE_READ = 0;
        static constexpr int PIPE_WRITE = 1;
        int pipe_in[2] = { 0 };
        int pipe_out[2] = { 0 };
        int pipe_in_ret = pipe(pipe_in);
        int pipe_out_ret = pipe(pipe_out);
        if (pipe_in_ret != 0 || pipe_out_ret != 0) {
            return {};
        }
        fcntl(pipe_out[PIPE_READ], F_SETFL, O_NONBLOCK);
        int exit_ret = 0;
        int child = fork();
        if (child == 0) {
            // child process
            dup2(pipe_in[PIPE_READ], STDIN_FILENO);
            dup2(pipe_out[PIPE_WRITE], STDOUT_FILENO);
            dup2(pipe_out[PIPE_WRITE], STDERR_FILENO);

            // all these are for use by parent only
            close(pipe_in[PIPE_READ]);
            close(pipe_in[PIPE_WRITE]);
            close(pipe_out[PIPE_READ]);
            close(pipe_out[PIPE_WRITE]);

            exit_ret = execlp("sh", "sh", "-c", cmdline.c_str(), nullptr);
            exit(exit_ret);
        }
        else if (child > 0) {
            // parent process

            // close unused file descriptors, these are for child only
            close(pipe_in[PIPE_READ]);
            close(pipe_out[PIPE_WRITE]);

            do {
                ssize_t read_num = read(pipe_out[PIPE_READ], pipe_buffer.get(), PipeBuffSize);

                while (read_num > 0) {
                    pipe_str.append(pipe_buffer.get(), pipe_buffer.get() + read_num);
                    read_num = read(pipe_out[PIPE_READ], pipe_buffer.get(), PipeBuffSize);
                };
            } while (::waitpid(child, &exit_ret, WNOHANG) == 0);

            close(pipe_in[PIPE_WRITE]);
            close(pipe_out[PIPE_READ]);
        }
        else {
            // failed to create child process
            close(pipe_in[PIPE_READ]);
            close(pipe_in[PIPE_WRITE]);
            close(pipe_out[PIPE_READ]);
            close(pipe_out[PIPE_WRITE]);
        }
#endif
        return pipe_str;
    }

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
} // namespace asst::utils
