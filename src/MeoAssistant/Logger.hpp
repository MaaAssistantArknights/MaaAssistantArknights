#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

#include "AsstRanges.hpp"
#include "AsstUtils.hpp"
#include "SingletonHolder.hpp"
#include "Version.h"

namespace asst
{
    class Logger : public SingletonHolder<Logger>
    {
    public:
        struct separator
        {
            constexpr separator() = default;
            constexpr separator(const separator&) = default;
            constexpr separator(separator&&) noexcept = default;
            constexpr explicit separator(std::string_view s) noexcept : str(s) {}
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
            constexpr explicit level(std::string_view s) noexcept : str(s) {}
            constexpr level& operator=(const level&) = default;
            constexpr level& operator=(level&&) noexcept = default;
            constexpr level& operator=(std::string_view s) noexcept
            {
                str = s;
                return *this;
            }

            static const level debug;
            static const level trace;
            static const level info;
            static const level warn;
            static const level error;

            std::string_view str;
        };

        class LogStream
        {
        public:
            LogStream(std::mutex& mtx, std::ofstream& ofs) : m_trace_lock(mtx), m_ofs(ofs) {}
            template <typename... Args>
            LogStream(std::mutex& mtx, std::ofstream& ofs, Args&&... buff) : m_trace_lock(mtx), m_ofs(ofs)
            {
                (*this << ... << std::forward<Args>(buff));
            }
            ~LogStream()
            {
                m_ofs << std::endl;
#ifdef ASST_DEBUG
                std::cout << std::endl;
#endif
            }

            template <typename T>
            LogStream& operator<<(T&& arg)
            {
                if constexpr (std::same_as<separator, std::decay_t<T>>) {
                    m_sep = std::forward<T>(arg);
                }
                else {
                    if (!m_is_first) {
#ifdef ASST_DEBUG
                        stream_put<true>(m_ofs, m_sep.str);
#else
                        stream_put<false>(m_ofs, m_sep.str);
#endif
                    }
                    else {
                        m_is_first = false;
                    }
#ifdef ASST_DEBUG
                    stream_put<true>(m_ofs, std::forward<T>(arg));
#else
                    stream_put<false>(m_ofs, std::forward<T>(arg));
#endif
                }
                return *this;
            }

        private:
            template <typename Stream, typename T, typename Enable = void>
            struct has_stream_insertion_operator : std::false_type
            {};

            template <typename Stream, typename T>
            struct has_stream_insertion_operator<Stream, T,
                                                 std::void_t<decltype(std::declval<Stream&>() << std::declval<T>())>>
                : std::true_type
            {};

            template <bool ToAnsi, typename Stream, typename T>
            static Stream& stream_put(Stream& s, T&& v)
            {
                if constexpr (std::same_as<std::filesystem::path, std::decay_t<T>>) {
                    if constexpr (ToAnsi)
                        s << utils::utf8_to_ansi(utils::path_to_utf8_string(std::forward<T>(v)));
                    else
                        s << utils::path_to_utf8_string(std::forward<T>(v));
                    return s;
                }
                else if constexpr (std::convertible_to<T, std::string_view>) {
                    if constexpr (ToAnsi)
                        s << utils::utf8_to_ansi(std::forward<T>(v));
                    else
                        s << std::forward<T>(v);
                    return s;
                }
                else if constexpr (has_stream_insertion_operator<Stream, T>::value) {
                    s << std::forward<T>(v);
                    return s;
                }
                else if constexpr (ranges::input_range<T>) {
                    s << "[";
                    std::string_view comma {};
                    for (const auto& elem : std::forward<T>(v)) {
                        s << comma;
                        stream_put<ToAnsi>(s, elem);
                        comma = ", ";
                    }
                    s << "]";
                    return s;
                }
                else {
                    ASST_STATIC_ASSERT_FALSE(
                        "unsupported type, one of the following expected\n"
                        "\t1. those can be converted to string;\n"
                        "\t2. those can be inserted to stream with operator<< directly;\n"
                        "\t3. container or nested container containing 1. 2. or 3. and iterable with range-based for",
                        Stream, T);
                    return s;
                }
            }

            bool m_is_first = true;
            separator m_sep { " " };
            std::unique_lock<std::mutex> m_trace_lock;
            std::ofstream& m_ofs;
        };

    public:
        virtual ~Logger() override { flush(); }

        static bool set_directory(const std::filesystem::path& dir)
        {
            if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
                return false;
            }
            m_directory = dir;

            return true;
        }

        template <typename... Args>
        inline void debug([[maybe_unused]] Args&&... args)
        {
#ifdef ASST_DEBUG
            log(level::debug, std::forward<Args>(args)...);
#endif
        }

        template <typename... Args>
        inline void trace(Args&&... args)
        {
            log(level::trace, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void info(Args&&... args)
        {
            log(level::info, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void warn(Args&&... args)
        {
            log(level::warn, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void error(Args&&... args)
        {
            log(level::error, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void log(level lv, Args&&... args)
        {
            ((*this << lv) << ... << std::forward<Args>(args));
        }

        void flush()
        {
            std::unique_lock<std::mutex> m_trace_lock(m_trace_mutex);
            if (m_ofs.is_open()) {
                m_ofs.close();
            }
        }

        template <typename T>
        LogStream operator<<(T&& arg)
        {
            level lv = level::trace;
            if constexpr (std::same_as<Logger::level, std::decay_t<T>>) {
                lv = std::forward<Logger::level>(arg);
            }
            constexpr int buff_len = 128;
            char buff[buff_len] = { 0 };
#ifdef _WIN32
#ifdef _MSC_VER
            sprintf_s(buff, buff_len,
#else  // ! _MSC_VER
            sprintf(buff,
#endif // END _MSC_VER
                      "[%s][%s][Px%x][Tx%lx]", asst::utils::get_format_time().c_str(), lv.str.data(), _getpid(),
                      ::GetCurrentThreadId());
#else  // ! _WIN32
            sprintf(buff, "[%s][%s][Px%x][Tx%lx]", asst::utils::get_format_time().c_str(), lv.str.data(), getpid(),
                    (unsigned long)(std::hash<std::thread::id> {}(std::this_thread::get_id())));
#endif // END _WIN32

            if (!m_ofs || !m_ofs.is_open()) {
                m_ofs = std::ofstream(m_log_path, std::ios::out | std::ios::app);
            }

            if constexpr (std::same_as<Logger::level, std::decay_t<T>>) {
                return LogStream(m_trace_mutex, m_ofs, buff);
            }
            else {
                return LogStream(m_trace_mutex, m_ofs, buff, arg);
            }
        }

    private:
        friend class SingletonHolder<Logger>;

        Logger()
        {
            check_filesize_and_remove();
            log_init_info();
        }

        void check_filesize_and_remove() const
        {
            constexpr uintmax_t MaxLogSize = 4ULL * 1024 * 1024;
            try {
                if (std::filesystem::exists(m_log_path)) {
                    const uintmax_t log_size = std::filesystem::file_size(m_log_path);
                    if (log_size >= MaxLogSize) {
                        std::filesystem::rename(m_log_path, m_log_bak_path);
                    }
                }
            }
            catch (...) {
            }
        }
        void log_init_info()
        {
            trace("-----------------------------");
            trace("MeoAssistant Process Start");
            trace("Version", asst::Version);
            trace("Built at", __DATE__, __TIME__);
            trace("Working Path", m_directory);
            trace("-----------------------------");
        }

        inline static std::filesystem::path m_directory;

        std::filesystem::path m_log_path = m_directory / "asst.log";
        std::filesystem::path m_log_bak_path = m_directory / "asst.bak.log";
        std::mutex m_trace_mutex;
        std::ofstream m_ofs;
    };

    inline const Logger::separator Logger::separator::none;
    inline const Logger::separator Logger::separator::space(" ");
    inline const Logger::separator Logger::separator::tab("\t");
    inline const Logger::separator Logger::separator::newline("\n");
    inline const Logger::separator Logger::separator::comma(",");

    inline const Logger::level Logger::level::debug("DBG");
    inline const Logger::level Logger::level::trace("TRC");
    inline const Logger::level Logger::level::info("INF");
    inline const Logger::level Logger::level::warn("WRN");
    inline const Logger::level Logger::level::error("ERR");

    class LoggerAux
    {
    public:
        explicit LoggerAux(std::string func_name)
            : m_func_name(std::move(func_name)), m_start_time(std::chrono::steady_clock::now())
        {
            Logger::get_instance().trace(m_func_name, "| enter");
        }
        ~LoggerAux()
        {
            const auto duration = std::chrono::steady_clock::now() - m_start_time;
            Logger::get_instance().trace(m_func_name, "| leave,",
                                         std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(), "ms");
        }
        LoggerAux(const LoggerAux&) = default;
        LoggerAux(LoggerAux&&) = default;
        LoggerAux& operator=(const LoggerAux&) = default;
        LoggerAux& operator=(LoggerAux&&) = default;

    private:
        std::string m_func_name;
        std::chrono::time_point<std::chrono::steady_clock> m_start_time;
    };

#define _Cat_(a, b) a##b
#define _Cat(a, b) _Cat_(a, b)
#define _CatVarNameWithLine(Var) _Cat(Var, __LINE__)

#define Log Logger::get_instance()
#define LogDebug Log << Logger::level::debug
#define LogTrace Log << Logger::level::trace
#define LogInfo Log << Logger::level::info
#define LogWarn Log << Logger::level::warn
#define LogError Log << Logger::level::error
#define LogTraceScope LoggerAux _CatVarNameWithLine(_func_aux_)
#define LogTraceFunction LogTraceScope(__FUNCTION__)
#define LogTraceFunctionWithArgs // how to do this?, like LogTraceScope(__FUNCTION__, __FUNCTION_ALL_ARGS__)
} // namespace asst
