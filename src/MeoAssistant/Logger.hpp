#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>
#include <thread>

#include "AsstUtils.hpp"
#include "Version.h"

namespace asst
{
    class Logger
    {
    public:
        ~Logger()
        {
            flush();
        }

        Logger(const Logger&) = delete;
        Logger(Logger&&) = delete;

        static Logger& get_instance()
        {
            static Logger _unique_instance;
            return _unique_instance;
        }

        static bool set_dirname(std::string dirname) noexcept
        {
            if (!std::filesystem::exists(dirname) || !std::filesystem::is_directory(dirname)) {
                return false;
            }
            m_dirname = std::move(dirname);
            return true;
        }

        template <typename... Args>
        inline void debug([[maybe_unused]] Args&&... args)
        {
#ifdef ASST_DEBUG
            std::string_view level = "DEB";
            log(level, std::forward<Args>(args)...);
#endif
        }

        template <typename... Args>
        inline void trace(Args&&... args)
        {
            std::string_view level = "TRC";
            log(level, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void info(Args&&... args)
        {
            std::string_view level = "INF";
            log(level, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void warn(Args&&... args)
        {
            std::string_view level = "WRN";
            log(level, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void error(Args&&... args)
        {
            std::string_view level = "ERR";
            log(level, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void log_with_custom_level(std::string_view level, Args&&... args)
        {
            log(level, std::forward<Args>(args)...);
        }
        void flush()
        {
            std::unique_lock<std::mutex> trace_lock(m_trace_mutex);
            if (m_ofs.is_open()) {
                m_ofs.close();
            }
        }

        const std::string m_log_filename = m_dirname + "asst.log";
        const std::string m_log_bak_filename = m_dirname + "asst.bak.log";

    private:
        Logger()
        {
            check_filesize_and_remove();
            log_init_info();
        }

        void check_filesize_and_remove()
        {
            constexpr uintmax_t MaxLogSize = 4 * 1024 * 1024;
            try {
                if (std::filesystem::exists(m_log_filename)) {
                    uintmax_t log_size = std::filesystem::file_size(m_log_filename);
                    if (log_size >= MaxLogSize) {
                        std::filesystem::rename(m_log_filename, m_log_bak_filename);
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
#ifdef _WIN32 // 输出到日志的时候统一编码utf8
            trace("Working Path", asst::utils::ansi_to_utf8(m_dirname));
#else
            trace("Working Path", m_dirname);
#endif
            trace("-----------------------------");
        }

        template <typename... Args>
        void log(std::string_view level, Args&&... args)
        {
            std::unique_lock<std::mutex> trace_lock(m_trace_mutex);

            constexpr int buff_len = 128;
            char buff[buff_len] = { 0 };
#ifdef _WIN32
#ifdef _MSC_VER
            sprintf_s(buff, buff_len,
#else   // ! _MSC_VER
            sprintf(buff,
#endif  // END _MSC_VER
            "[%s][%s][Px%x][Tx%x]",
                      asst::utils::get_format_time().c_str(),
                      level.data(), _getpid(), ::GetCurrentThreadId()
            );
#else   // ! _WIN32
            sprintf(buff, "[%s][%s][Px%x][Tx%x]",
                      asst::utils::get_format_time().c_str(),
                      level.data(), getpid(), unsigned (std::hash<std::thread::id>{}(std::this_thread::get_id()))
            );
#endif  // END _WIN32

            if (!m_ofs || !m_ofs.is_open()) {
                m_ofs = std::ofstream(m_log_filename, std::ios::out | std::ios::app);
            }
#ifdef ASST_DEBUG
            stream_put_line(m_ofs, buff, args...);
#else
            stream_put_line<false>(m_ofs, buff, std::forward<Args>(args)...);
#endif

#ifdef ASST_DEBUG
            stream_put_line<true>(std::cout, buff, std::forward<Args>(args)...);
#endif
        }

        template <bool ToAnsi, typename Stream, typename T, typename Enable = void>
        struct stream_put_converted_impl
        {
            static constexpr Stream& apply(Stream& s, T&& value)
            {
                s << std::forward<T>(value);
                return s;
            }
        };

        template <typename Stream, typename T>
        struct stream_put_converted_impl<true, Stream, T, std::enable_if_t<std::is_constructible_v<std::string, T>>>
        {
            static constexpr Stream& apply(Stream& s, T&& value)
            {
#ifdef _WIN32
                s << utils::utf8_to_ansi(std::forward<T>(value));
#endif
                return s;
            }
        };

        template <bool ToAnsi, typename Stream, typename... Args>
        struct stream_put_line_impl;

        template <bool ToAnsi, typename Stream>
        struct stream_put_line_impl <ToAnsi, Stream>{
            static constexpr Stream& apply(Stream& s) {
                s << std::endl;
                return s;
            }
        };

        template <bool ToAnsi, typename Stream, typename Only>
        struct stream_put_line_impl<ToAnsi, Stream, Only>
        {
            static constexpr Stream& apply(Stream& s, Only&& only)
            {
                stream_put_converted_impl<false, Stream, Only>::apply(s, std::forward<Only>(only));
                s << std::endl;
                return s;
            }
        };

        template <bool ToAnsi, typename Stream, typename First, typename... Rest>
        struct stream_put_line_impl<ToAnsi, Stream, First, Rest...>
        {
            static constexpr Stream& apply(Stream& s, First f, Rest... rs)
            {
                stream_put_converted_impl<ToAnsi, Stream, First>::apply(s, std::forward<First>(f));
                s << " ";
                stream_put_line_impl<ToAnsi, Stream, Rest...>::apply(s, std::forward<Rest>(rs)...);
                return s;
            }
        };

        template <bool ToAnsi = false, typename Stream, typename... Args>
        Stream& stream_put_line(Stream& s, Args... args)
        {
            return stream_put_line_impl<ToAnsi, Stream, Args...>::apply(s, std::forward<Args>(args)...);
        }

        inline static std::string m_dirname;
        std::mutex m_trace_mutex;
        std::ofstream m_ofs;
    };

    class LoggerAux
    {
    public:
        LoggerAux(std::string func_name)
            : m_func_name(std::move(func_name)),
            m_start_time(std::chrono::steady_clock::now())
        {
            Logger::get_instance().trace(m_func_name, "| enter");
        }
        ~LoggerAux()
        {
            auto duration = std::chrono::steady_clock::now() - m_start_time;
            Logger::get_instance().trace(m_func_name, "| leave,",
                                         std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(), "ms");
        }

    private:
        std::string m_func_name;
        std::chrono::time_point<std::chrono::steady_clock> m_start_time;
    };

#define _Cat_(a, b) a ## b
#define _Cat(a, b) _Cat_(a, b)
#define _CatVarNameWithLine(Var) _Cat(Var, __LINE__)

#define Log Logger::get_instance()
#define LogTraceScope LoggerAux _CatVarNameWithLine(_func_aux_)
#define LogTraceFunction LogTraceScope(__FUNCTION__)
#define LogTraceFunctionWithArgs // how to do this?, like LogTraceScope(__FUNCTION__, __FUNCTION_ALL_ARGS__)
}
