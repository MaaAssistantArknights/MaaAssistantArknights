#pragma once

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#include <csignal>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <ranges>
#include <streambuf>
#include <thread>
#include <type_traits>
#include <utility>

#include "Common/AsstTypes.h"
#include "Locale.hpp"
#include "MaaUtils/Conf.h"
#include "MaaUtils/SingletonHolder.hpp"
#include "MaaUtils/Time.hpp"
#include "Meta.hpp"
#include "NullStreambuf.hpp"
#include "Platform.hpp"
#include "WorkingDir.hpp"

#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#endif

namespace asst
{
template <typename Stream, typename T>
concept has_stream_insertion_operator = requires { std::declval<Stream&>() << std::declval<T>(); };
template <typename T>
concept enum_could_to_string = requires { asst::enum_to_string(std::declval<T>()); };

// is_reference_wrapper
template <typename T>
struct is_reference_wrapper : std::false_type
{
};

template <typename T>
struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type
{
};

template <typename T>
struct is_reference_wrapper<std::reference_wrapper<T>&> : std::true_type
{
};

template <typename T>
struct is_reference_wrapper<std::reference_wrapper<T>&&> : std::true_type
{
};

template <typename T>
inline constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;

// from_reference_wrapper
template <typename T>
struct from_reference_wrapper
{
    typedef T type;
};

template <typename T>
struct from_reference_wrapper<std::reference_wrapper<T>>
{
    typedef typename from_reference_wrapper<T&>::type type;
};

template <typename T>
struct from_reference_wrapper<std::reference_wrapper<T>&>
{
    typedef typename from_reference_wrapper<T&>::type type;
};

template <typename T>
struct from_reference_wrapper<std::reference_wrapper<T>&&>
{
    typedef typename from_reference_wrapper<T&>::type type;
};

template <typename T>
using from_reference_wrapper_t = typename from_reference_wrapper<T>::type;

// to_reference_wrapper
template <typename T>
struct to_reference_wrapper
{
    typedef T type;
};

template <typename T>
struct to_reference_wrapper<T&>
{
    typedef std::reference_wrapper<T> type;
};

template <typename T>
struct to_reference_wrapper<std::reference_wrapper<T>>
{
    typedef typename to_reference_wrapper<T&>::type type;
};

template <typename T>
struct to_reference_wrapper<std::reference_wrapper<T>&>
{
    typedef typename to_reference_wrapper<T&>::type type;
};

template <typename T>
struct to_reference_wrapper<std::reference_wrapper<T>&&>
{
    typedef typename to_reference_wrapper<T&>::type type;
};

template <typename T>
using to_reference_wrapper_t = typename to_reference_wrapper<T>::type;

template <typename T>
struct remove_cvref
{
    typedef std::remove_cvref_t<from_reference_wrapper_t<std::remove_cvref_t<T>>> type;
};

template <typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

template <typename T>
constexpr from_reference_wrapper_t<T> convert_reference_wrapper(T&& x)
{
    return x;
}

namespace detail
{
class scope_slice
{
public:
    using id = int;

    scope_slice() = default;

    std::string push(id& i)
    {
        i = 0;
        if (auto iter = std::find_if(m_state.rbegin(), m_state.rend(), [](int e) { return e != -1; });
            iter != m_state.rend()) {
            i = *iter + 1;
        }

        std::string result;
        result.reserve(m_state.size() * 3);
        for (auto e : m_state) {
            result += (e == -1 ? " " : "│");
        }

        m_state.push_back(i);
        result += "┌";

        return result;
    }

    std::string pop(id i)
    {
        std::string result;
        result.reserve(m_state.size() * 3);
        std::string sym = "│";
        std::string emp = " ";
        for (auto& e : m_state) {
            if (i != -1 && e == i) {
                result += "└";
                e = -1;
                sym = "┼";
                emp = "─";
            }
            else {
                result += (e == -1 ? emp : sym);
            }
        }

        while (!m_state.empty() && m_state.back() == -1) {
            m_state.pop_back();
        }
        return result;
    }

    std::string next()
    {
        while (!m_state.empty() && m_state.back() == -1) {
            m_state.pop_back();
        }

        std::string result;
        result.reserve(m_state.size() * 3);
        auto lhs = m_state.end();
        auto rhs = m_state.end();
        auto iter = m_state.begin();
        //      | <- rhs
        // ------
        // | <- lhs
        for (; iter != m_state.end(); ++iter) {
            if (*iter != -1) {
                result += "│";
                continue;
            }

            lhs = iter;
            result += "┌";
            // if (iter != m_state.end())
            ++iter;

            for (; iter != m_state.end(); ++iter) {
                if (*iter == -1) {
                    result += "─";
                }
                else {
                    break;
                }
            }
            rhs = iter;
            result += "┘";

            if (iter != m_state.end()) {
                ++iter;
            }

            for (; iter != m_state.end(); ++iter) {
                result += (*iter == -1 ? " " : "│");
            }
            break;
        }

        if (lhs != m_state.end()) {
            if (rhs == m_state.end()) {
                m_state.erase(lhs, rhs);
            }
            else {
                std::swap(*lhs, *rhs);
            }
        }

        return result;
    }

private:
    std::vector<id> m_state { };
};
} // namespace detail

class console_ostream
{
    std::reference_wrapper<std::ostream> m_ofs;

public:
    console_ostream(console_ostream&&) = default;
    console_ostream(const console_ostream&) = default;
    console_ostream& operator=(console_ostream&&) = default;
    console_ostream& operator=(const console_ostream&) = default;

    console_ostream(std::ostream& stream) :
        m_ofs(stream)
    {
    }

    console_ostream(std::reference_wrapper<std::ostream> stream) :
        m_ofs(stream)
    {
    }

    template <typename T>
    requires has_stream_insertion_operator<std::ostream, T>
    console_ostream& operator<<(T&& v)
    {
#ifdef _WIN32
        if constexpr (std::convertible_to<T, std::string_view>) {
            asst::utils::utf8_scope scope(m_ofs.get());
            m_ofs.get() << std::forward<T>(v);
        }
        else {
            m_ofs.get() << std::forward<T>(v);
        }
#else
        m_ofs.get() << std::forward<T>(v);
#endif
        return *this;
    }

    console_ostream& operator<<(std::ostream& (*pf)(std::ostream&))
    {
        m_ofs.get() << pf;
        return *this;
    }
};

template <typename... Args>
class ostreams
{
    std::tuple<Args...> m_ofss;

public:
    ostreams(ostreams&&) = default;
    ostreams(const ostreams&) = default;
    ostreams& operator=(ostreams&&) = default;
    ostreams& operator=(const ostreams&) = default;

    ostreams(Args&&... args) :
        m_ofss(std::forward<Args>(args)...)
    {
    }

    template <typename T>
    requires has_stream_insertion_operator<std::ostream, T>
    ostreams& operator<<(T&& x)
    {
        streams_put(m_ofss, x, std::index_sequence_for<Args...> { });
        return *this;
    }

    template <typename Tuple, typename T, size_t... Is>
    static void streams_put(Tuple& t, T&& x, std::index_sequence<Is...>)
    {
        ((convert_reference_wrapper(std::get<Is>(t)) << std::forward<T>(x)), ...);
    }

    ostreams& operator<<(std::ostream& (*pf)(std::ostream&))
    {
        streams_put(m_ofss, pf, std::index_sequence_for<Args...> { });
        return *this;
    }

    template <typename Tuple, size_t... Is>
    static void streams_put(Tuple& t, std::ostream& (*pf)(std::ostream&), std::index_sequence<Is...>)
    {
        ((convert_reference_wrapper(std::get<Is>(t)) << pf), ...);
    }
};

template <typename... Args>
ostreams(Args&&...) -> ostreams<to_reference_wrapper_t<Args>...>;

class Logger : public MAA_NS::SingletonHolder<Logger>
{
public:
    struct separator
    {
        constexpr separator() = default;
        constexpr separator(const separator&) = default;
        constexpr separator(separator&&) noexcept = default;

        constexpr explicit separator(std::string_view s) noexcept :
            str(s)
        {
        }

        constexpr separator& operator=(const separator&) = default;
        constexpr separator& operator=(separator&&) noexcept = default;

        constexpr separator& operator=(std::string_view s) noexcept
        {
            str = s;
            return *this;
        }

        static const separator none;
        static const separator space;
        static const separator tab;
        static const separator newline;
        static const separator comma;

        std::string_view str;
    };

    struct level
    {
        constexpr level(const level&) = default;
        constexpr level(level&&) noexcept = default;

        constexpr explicit level(std::string_view s) noexcept :
            str(s)
        {
        }

        constexpr level& operator=(const level&) = default;
        constexpr level& operator=(level&&) noexcept = default;

        constexpr level& operator=(std::string_view s) noexcept
        {
            str = s;
            return *this;
        }

        bool enabled = true;

        bool is_enabled() const { return enabled; }

        void set_enabled(bool enable) { enabled = enable; }

        static level debug;
        static level trace;
        static level info;
        static level warn;
        static level error;

        std::string_view str;
    };

    template <typename stream_t>
    class LogStream
    {
    public:
        template <typename T>
        LogStream& operator<<(T&& arg)
        {
            if constexpr (std::same_as<separator, remove_cvref_t<T>>) {
                m_sep = std::forward<T>(arg);
            }
            else {
                // 如果是 level，则不输出 separator
                if constexpr (!std::same_as<Logger::level, remove_cvref_t<T>>) {
                    stream_put(m_ofs, m_sep.str);
                }
                stream_put(m_ofs, std::forward<T>(arg));
            }
            return *this;
        }

        template <typename _stream_t = stream_t>
        LogStream(std::mutex& mtx, _stream_t&& ofs, Logger::level lv) :
            m_trace_lock(mtx),
            m_ofs(ofs)
        {
            *this << lv;
        }

        template <typename _stream_t = stream_t, typename... Args>
        LogStream(std::mutex& mtx, _stream_t&& ofs, Logger::level lv, Args&&... buff) :
            m_trace_lock(mtx),
            m_ofs(ofs)
        {
            ((*this << lv) << ... << std::forward<Args>(buff));
        }

        template <typename _stream_t = stream_t, typename... Args>
        LogStream(std::unique_lock<std::mutex>&& lock, _stream_t&& ofs, Logger::level lv, Args&&... buff) :
            m_trace_lock(std::move(lock)),
            m_ofs(ofs)
        {
            ((*this << lv) << ... << std::forward<Args>(buff));
        }

        LogStream(LogStream&&) = delete;
        LogStream(const LogStream&) = delete;
        LogStream& operator=(LogStream&&) = delete;
        LogStream& operator=(const LogStream&) = delete;

        ~LogStream() { m_ofs << std::endl; }

    private:
        template <typename Stream, typename T>
        static Stream& stream_put(Stream& s, T&& v)
        {
            if constexpr (std::same_as<std::filesystem::path, remove_cvref_t<T>>) {
                s << utils::path_to_utf8_string(std::forward<T>(v));
            }
            else if constexpr (std::same_as<Logger::level, remove_cvref_t<T>>) {
#ifdef _WIN32
                int pid = _getpid();
#else
                int pid = ::getpid();
#endif
                auto tid = static_cast<uint16_t>(std::hash<std::thread::id> { }(std::this_thread::get_id()));

                s << std::format("[{}][{}][Px{}][Tx{}]", MAA_NS::format_now(), v.str, pid, tid);
            }
            else if constexpr (std::is_enum_v<T> && enum_could_to_string<T>) {
                s << asst::enum_to_string(std::forward<T>(v));
            }
            else if constexpr (has_stream_insertion_operator<Stream, T>) {
                s << std::forward<T>(v);
            }
            else if constexpr (std::constructible_from<std::string, T>) {
                s << std::string(std::forward<T>(v));
            }
            else if constexpr (std::ranges::input_range<T>) {
                s << "[";
                std::string_view comma_space { };
                for (const auto& elem : std::forward<T>(v)) {
                    s << comma_space;
                    stream_put(s, elem);
                    comma_space = ", ";
                }
                s << "]";
            }
            else {
                ASST_STATIC_ASSERT_FALSE(
                    "unsupported type, one of the following expected\n"
                    "\t1. those can be converted to string;\n"
                    "\t2. those can be inserted to stream with operator<< directly;\n"
                    "\t3. container or nested container containing 1. 2. or 3. and iterable with "
                    "range-based for",
                    Stream,
                    T);
            }
            return s;
        }

        separator m_sep = separator::space;
        std::unique_lock<std::mutex> m_trace_lock;
        stream_t m_ofs;
    };

    // template <typename stream_t>
    // LogStream(std::mutex&, stream_t&) -> LogStream<stream_t&>;

    // template <typename stream_t>
    // LogStream(std::mutex&, stream_t&&) -> LogStream<stream_t>;

    template <typename stream_t, typename... Args>
    LogStream(std::mutex&, stream_t&, Args&&...) -> LogStream<stream_t&>;

    template <typename stream_t, typename... Args>
    LogStream(std::mutex&, stream_t&&, Args&&...) -> LogStream<stream_t>;

    template <typename stream_t, typename... Args>
    LogStream(std::unique_lock<std::mutex>&&, stream_t&&, Args&&...) -> LogStream<stream_t>;

    class LogStreambuf : public std::filebuf
    {
    public:
        LogStreambuf(std::filebuf* dest_buf) :
            dest(dest_buf)
        {
        }

        std::streamsize count_bytes() const { return count; }

    protected:
        int_type overflow(int_type c) override
        {
            if (c != traits_type::eof()) {
                ch = static_cast<char>(c);
                count++;
                if (dest) {
                    dest->sputc(ch);
                }
            }
            return ch;
        }

        int sync() override
        {
            if (dest) {
                return dest->pubsync();
            }
            return 0;
        }

        // 处理字符块
        std::streamsize xsputn(const char* s, std::streamsize n) override
        {
            count += n;
            if (dest) {
                return dest->sputn(s, n);
            }
            return n;
        }

    private:
        char ch = 0;
        std::filebuf* dest;
        std::streamsize count = 0;
    };

public:
    virtual ~Logger() override { flush(); }

    // static bool set_directory(const std::filesystem::path& dir)
    // {
    //     if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
    //         return false;
    //     }
    //     m_directory = dir;

    //     return true;
    // }

    template <typename T>
    auto operator<<(T&& arg)
    {
        std::unique_lock lock { m_trace_mutex };
        rotate();
        if constexpr (std::same_as<level, remove_cvref_t<T>>) {
#ifdef ASST_DEBUG
            return LogStream(std::move(lock), ostreams { console_ostream(std::cout), m_of }, arg);
#else
            return LogStream(std::move(lock), m_of, arg);
#endif
        }
        else {
#ifdef ASST_DEBUG
            return LogStream(std::move(lock), ostreams { console_ostream(std::cout), m_of }, level::trace, arg);
#else
            return LogStream(std::move(lock), m_of, level::trace, arg);
#endif
        }
    }

#ifdef ASST_DEBUG
#define LOGGER_FUNC_WITH_LEVEL(lv)                                                     \
    template <typename... Args>                                                        \
    inline void lv(Args&&... args)                                                     \
    {                                                                                  \
        std::unique_lock lock { m_trace_mutex };                                       \
        log(std::move(lock), level::lv, m_scopes.next(), std::forward<Args>(args)...); \
    }
#else
#define LOGGER_FUNC_WITH_LEVEL(lv)                   \
    template <typename... Args>                      \
    inline void lv(Args&&... args)                   \
    {                                                \
        log(level::lv, std::forward<Args>(args)...); \
    }
#endif

    LOGGER_FUNC_WITH_LEVEL(trace)
    LOGGER_FUNC_WITH_LEVEL(info)
    LOGGER_FUNC_WITH_LEVEL(warn)
    LOGGER_FUNC_WITH_LEVEL(error)

    template <typename... Args>
    inline void debug([[maybe_unused]] Args&&... args)
    {
#ifndef ASST_DEBUG
        static const bool need_log = std::filesystem::exists("DEBUG.txt");
        if (!need_log) {
            return;
        }
#endif
        std::unique_lock lock { m_trace_mutex };
        log(std::move(lock), level::debug, m_scopes.next(), std::forward<Args>(args)...);
    }

#undef LOGGER_FUNC_WITH_LEVEL

    template <typename... args_t>
    auto error_(args_t&&... args)
    {
        return stream(level::error, m_scopes.next(), std::forward<args_t>(args)...);
    }

    template <typename... args_t>
    auto warn_(args_t&&... args)
    {
        return stream(level::warn, m_scopes.next(), std::forward<args_t>(args)...);
    }

    template <typename... args_t>
    auto info_(args_t&&... args)
    {
        return stream(level::info, m_scopes.next(), std::forward<args_t>(args)...);
    }

    template <typename... args_t>
    auto debug_(args_t&&... args)
    {
#ifndef ASST_DEBUG
        static const bool need_log = std::filesystem::exists("DEBUG.txt");
        if (!need_log) {
            return LogStream(
                std::unique_lock { m_trace_mutex },
                null_stream,
                level::debug,
                std::forward<args_t>(args)...);
        }
#endif
        return stream(level::debug, m_scopes.next(), std::forward<args_t>(args)...);
    }

    template <typename... args_t>
    auto trace_(args_t&&... args)
    {
        return stream(level::trace, m_scopes.next(), std::forward<args_t>(args)...);
    }

    template <typename... Args>
    inline int push(Args&&... args)
    {
        int id = -1;
        std::unique_lock lock { m_trace_mutex };
        log(std::move(lock), level::trace, m_scopes.push(id), std::forward<Args>(args)...);
        return id;
    }

    template <typename... Args>
    inline void pop(int id, Args&&... args)
    {
        std::unique_lock lock { m_trace_mutex };
        log(std::move(lock), level::trace, m_scopes.pop(id), std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void log(level lv, Args&&... args)
    {
        if (lv.is_enabled()) {
            ((*this << lv) << ... << std::forward<Args>(args));
        }
    }

    template <typename... Args>
    inline void log(std::unique_lock<std::mutex>&& lock, level lv, Args&&... args)
    {
        if (!lv.is_enabled()) {
            return;
        }
        rotate();
#ifdef ASST_DEBUG
        (LogStream(std::move(lock), ostreams { console_ostream(std::cout), m_of }, lv)
         << ... << std::forward<Args>(args));
#else
        (LogStream(std::move(lock), m_of, lv) << ... << std::forward<Args>(args));

#endif
    }

    void flush()
    {
        std::unique_lock<std::mutex> m_trace_lock(m_trace_mutex);
        if (m_ofs.is_open()) {
            m_of.flush();
            m_ofs.close();
        }
    }

private:
    friend class MAA_NS::SingletonHolder<Logger>;

    Logger() :
        m_directory(UserDir.get()),
        m_buff(nullptr),
        m_of(&m_buff)
    {
        initialize_exception_handlers();

        try {
            std::filesystem::create_directories(m_log_path.parent_path());
        }
        catch (std::filesystem::filesystem_error& e) {
            std::cerr << e.what() << std::endl;
        }
        catch (...) {
        }

        LoadFileStream();
        log_init_info();
    }

    void rotate()
    {
        if (!m_of || !m_ofs || !m_ofs.is_open()) {
            LoadFileStream();
        }
        if (m_file_size + m_buff.count_bytes() <= MaxLogSize) {
            return;
        }
        try {
            if (!std::filesystem::exists(m_log_path) || !std::filesystem::is_regular_file(m_log_path)) {
                return;
            }
            m_buff.close();
            m_ofs.close();
            std::filesystem::rename(m_log_path, m_log_bak_path);
        }
        catch (std::filesystem::filesystem_error& e) {
            std::cerr << e.what() << std::endl;
        }
        catch (...) {
        }
        LoadFileStream();
    }

    void LoadFileStream()
    {
#ifdef _WIN32
        FILE* fp = nullptr;
        int file_handle = -1;
        int oflag = _O_WRONLY | _O_APPEND | _O_CREAT | _O_BINARY | _O_NOINHERIT; // 写入、追加、创建、二进制、不可继承
        int shflag = _SH_DENYWR;                                                 // 允许其他进程读取但不能写入
        int pmode = _S_IREAD | _S_IWRITE;                                        // 读写权限

        // 打开文件
        if (_sopen_s(&file_handle, utils::path_to_utf8_string(m_log_path).c_str(), oflag, shflag, pmode) == 0) {
            // 文件打开成功，转换为FILE*指针
            fp = _fdopen(file_handle, "a"); // 使用追加模式
            if (!fp) {
                // 如果转换失败，关闭文件句柄
                _close(file_handle);
            }
        }

        if (!fp) {
            // 打开失败时回退到原始方法
            m_ofs = std::ofstream(m_log_path, std::ios::out | std::ios::app);
        }
        else {
            // 使用文件指针创建新的std::ofstream
            // 注意：传递文件指针的所有权给ofstream
            m_ofs = std::ofstream(fp);

            // 如果需要，这里还可以添加一个安全检查
            if (!m_ofs) {
                fclose(fp);
                m_ofs = std::ofstream(m_log_path, std::ios::out | std::ios::app);
            }
        }
#else
        m_ofs = std::ofstream(m_log_path, std::ios::out | std::ios::app);
#endif
        // 获取文件大小并设置缓冲区
        m_file_size = std::filesystem::exists(m_log_path) ? std::filesystem::file_size(m_log_path) : 0;
        m_buff = LogStreambuf(m_ofs.rdbuf());
        m_of.rdbuf(&m_buff);
    }

    void log_init_info()
    {
        trace("-----------------------------");
        trace("MaaCore Process Start");
        trace("Version", MAA_VERSION);
        trace("Built at", __DATE__, __TIME__);
        trace("User Dir", m_directory);
        trace("-----------------------------");
    }

    inline static std::atomic<const char*> g_last_signal_reason { nullptr };

    static void write_crash_file(const char* reason, const char* detail = nullptr) noexcept
    {
        FILE* f = nullptr;
#ifdef ASST_DEBUG
        auto path = (UserDir.get() / "debug" / "crash.log").string();
#else
        auto path = (UserDir.get() / "crash.log").string();
#endif // ASST_DEBUG
#ifdef _WIN32
        if (fopen_s(&f, path.c_str(), "a") != 0) {
            return;
        }
#else
        f = fopen(path.c_str(), "a");
#endif // _WIN32
        if (!f) {
            return;
        }
        fprintf(f, "=== FATAL ERROR ===\n");
        if (reason) {
            fprintf(f, "Reason: %s\n", reason);
        }
        if (detail) {
            fprintf(f, "Detail: %s\n", detail);
        }
        fprintf(f, "===================\n\n");
        fflush(f);
        fclose(f);
    }

#ifdef _WIN32
    // SEH 未处理异常过滤器
    static LONG WINAPI unhandled_exception_filter([[maybe_unused]] PEXCEPTION_POINTERS pExceptionInfo)
    {
        try {
            auto& logger = Logger::get_instance();
            logger.error("=== UNHANDLED EXCEPTION ===");
            logger.error("Version", MAA_VERSION);
            logger.error("Built at", __DATE__, __TIME__);
            logger.error("User Dir", UserDir.get());
            logger.error("============================");
            logger.flush();
            write_crash_file("UNHANDLED EXCEPTION");
        }
        catch (...) {
            std::cerr << "=== FATAL ERROR ===" << std::endl;
            std::cerr << "Failed to log exception details to file" << std::endl;
            std::cerr << "Unhandled exception caught, program terminating..." << std::endl;
            std::cerr << "===================" << std::endl;
        }

        // 返回 EXCEPTION_EXECUTE_HANDLER 让程序正常终止
        return EXCEPTION_EXECUTE_HANDLER;
    }
#endif

    static void custom_terminate_handler() noexcept
    {
        static bool in_handler = false;
        if (in_handler) {
            std::_Exit(EXIT_FAILURE); // 避免递归
        }
        in_handler = true;

        try {
            auto& logger = Logger::get_instance();

            // 先写信号信息
            if (auto sig_reason = g_last_signal_reason.load()) {
                logger.error("=== FATAL ERROR ===");
                logger.error("Signal caught:", sig_reason);
                logger.flush();
                write_crash_file("Fatal Signal", sig_reason);
            }

            // 再处理 C++ 异常
            std::string exception_info = "Unknown exception";
            if (auto eptr = std::current_exception()) {
                try {
                    std::rethrow_exception(eptr);
                }
                catch (const std::exception& e) {
                    exception_info = std::string("std::exception: ") + e.what() + " (type: " + typeid(e).name() + ")";
                }
                catch (...) {
                    exception_info = "Unknown exception type";
                }
            }

            logger.error("=== FATAL ERROR ===");
            logger.error("Version", MAA_VERSION);
            logger.error("Built at", __DATE__, __TIME__);
            logger.error("User Dir", UserDir.get());
            logger.error("Unhandled exception caught:", exception_info);
            logger.error("Program terminating...");
            logger.error("===================");
            logger.flush();
            write_crash_file("Unhandled exception", exception_info.c_str());
        }
        catch (...) {
            std::cerr << "=== FATAL ERROR ===" << std::endl;
            std::cerr << "Failed to log exception details to file" << std::endl;
            std::cerr << "Unhandled exception caught, program terminating..." << std::endl;
            std::cerr << "===================" << std::endl;
        }
    }

    static void signal_handler(int sig)
    {
        std::string sig_name;
        switch (sig) {
        case SIGSEGV:
            sig_name = "SIGSEGV (Segmentation Fault)";
            break;
        case SIGABRT:
            sig_name = "SIGABRT (Abort)";
            break;
        case SIGFPE:
            sig_name = "SIGFPE (Floating Point Error)";
            break;
        case SIGILL:
            sig_name = "SIGILL (Illegal Instruction)";
            break;
        default:
            sig_name = "Signal " + std::to_string(sig);
            break;
        }
        g_last_signal_reason.store(sig_name.c_str());
        custom_terminate_handler();
        std::_Exit(EXIT_FAILURE);
    }

    static void initialize_exception_handlers()
    {
#ifdef _WIN32
        // Windows: 设置未处理异常过滤器
        SetUnhandledExceptionFilter(unhandled_exception_filter);
#endif
// android not need this
#ifndef __ANDROID__
        std::signal(SIGSEGV, signal_handler);
        std::signal(SIGABRT, signal_handler);
        std::signal(SIGFPE, signal_handler);
        std::signal(SIGILL, signal_handler);
#endif

#ifdef ASST_DEBUG
        const auto& path = UserDir.get() / "debug" / "crash.log";
        if (std::filesystem::exists(path)) {
            std::filesystem::remove(path);
        }
#endif // ASST_DEBUG
    }

    template <typename... args_t>
    auto stream(level lv, args_t&&... args)
    {
        rotate();
        if (lv.is_enabled()) {
#ifdef ASST_DEBUG
            return LogStream(
                std::unique_lock { m_trace_mutex },
                ostreams { console_ostream(std::cout), m_of },
                lv,
                std::forward<args_t>(args)...);
#else
            return LogStream(std::unique_lock { m_trace_mutex }, m_of, lv, std::forward<args_t>(args)...);
#endif
        }
        else {
#ifdef ASST_DEBUG
            return LogStream(
                std::unique_lock { m_trace_mutex },
                ostreams { console_ostream(std::cout), null_stream },
                lv,
                std::forward<args_t>(args)...);
#else
            return LogStream(std::unique_lock { m_trace_mutex }, null_stream, lv, std::forward<args_t>(args)...);
#endif
        }
    }

    detail::scope_slice m_scopes;

    std::filesystem::path m_directory;

    std::filesystem::path m_log_path = m_directory / "debug" / "asst.log";
    std::filesystem::path m_log_bak_path = m_directory / "debug" / "asst.bak.log";
    std::mutex m_trace_mutex;
    std::ofstream m_ofs;
    LogStreambuf m_buff;
    std::ostream m_of;
    std::size_t m_file_size = 0;

    static inline utils::NullStreambuf null_buf { };
    static inline std::ostream null_stream { &null_buf };
    const std::size_t MaxLogSize = 64LL * 1024 * 1024;
};

inline constexpr Logger::separator Logger::separator::none;
inline constexpr Logger::separator Logger::separator::space(" ");
inline constexpr Logger::separator Logger::separator::tab("\t");
inline constexpr Logger::separator Logger::separator::newline("\n");
inline constexpr Logger::separator Logger::separator::comma(",");

inline Logger::level Logger::level::debug("DBG");
inline Logger::level Logger::level::trace("TRC");
inline Logger::level Logger::level::info("INF");
inline Logger::level Logger::level::warn("WRN");
inline Logger::level Logger::level::error("ERR");

class LoggerAux
{
public:
    explicit LoggerAux(std::string_view func_name) :
        m_func_name(func_name),
        m_start_time(std::chrono::steady_clock::now())
    {
#ifdef ASST_DEBUG
        m_id = Logger::get_instance().push
#else
        Logger::get_instance().trace
#endif
               (m_func_name, "| enter");
    }

    ~LoggerAux()
    {
        const auto duration = std::chrono::steady_clock::now() - m_start_time;
#ifdef ASST_DEBUG
        Logger::get_instance().pop(
            m_id,
#else
        Logger::get_instance().trace(
#endif
            m_func_name,
            "| leave,",
            std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(),
            "ms");
    }

    LoggerAux(const LoggerAux&) = default;
    LoggerAux(LoggerAux&&) = default;
    LoggerAux& operator=(const LoggerAux&) = default;
    LoggerAux& operator=(LoggerAux&&) = default;

private:
    std::string m_func_name;
    std::chrono::time_point<std::chrono::steady_clock> m_start_time;
    int m_id [[maybe_unused]] = -1;
};

#define _Cat_(a, b) a##b
#define _Cat(a, b) _Cat_(a, b)
#define _CatVarNameWithLine(Var) _Cat(Var, __LINE__)

#define Log asst::Logger::get_instance()
#define LogDebug Log.debug_()
#define LogTrace Log.trace_()
#define LogInfo Log.info_()
#define LogWarn Log.warn_()
#define LogError Log.error_()

#define LogTraceScope LoggerAux _CatVarNameWithLine(_func_aux_)

#ifndef _MSC_VER
inline constexpr std::string_view summarize_pretty_function(std::string_view pf) // can be consteval?
{
    // unable to handle something like std::function<void(void)> gen_func()
    const auto paren = pf.find_first_of('(');
    if (paren != std::string_view::npos) {
        pf.remove_suffix(pf.size() - paren);
    }
    const auto space = pf.find_last_of(' ');
    if (space != std::string_view::npos) {
        pf.remove_prefix(space + 1);
    }
    return pf;
}

// TODO: use std::source_location
#define LogTraceFunction                                       \
    LogTraceScope                                              \
    {                                                          \
        ::asst::summarize_pretty_function(__PRETTY_FUNCTION__) \
    }
#else
#define LogTraceFunction LogTraceScope(__FUNCTION__)
#endif

#define VAR_RAW(x) "[" << #x << "=" << (x) << "] "
#define VAR(x) Logger::separator::none << VAR_RAW(x) << Logger::separator::space
#define VAR_VOIDP_RAW(x) "[" << #x << "=" << reinterpret_cast<void*>(x) << "] "
#define VAR_VOIDP(x) Logger::separator::none << VAR_VOIDP_RAW(x) << Logger::separator::space

#define LogTraceFunctionWithArgs // how to do this?, like LogTraceScope(__FUNCTION__,
                                 // __FUNCTION_ALL_ARGS__)
} // namespace asst
