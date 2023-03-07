#ifndef _WIN32
#include "ControllerPosix.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/errno.h>
#include <sys/socket.h>
#ifndef __APPLE__
#include <sys/prctl.h>
#endif
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>

#include "Common/AsstTypes.h"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"

asst::ControllerPosix::ControllerPosix(Assistant* inst) : InstHelper(inst)
{
    LogTraceFunction;

    int pipe_in_ret = ::pipe(m_pipe_in);
    int pipe_out_ret = ::pipe(m_pipe_out);
    ::fcntl(m_pipe_out[PIPE_READ], F_SETFL, O_NONBLOCK);

    if (pipe_in_ret < 0 || pipe_out_ret < 0) {
        Log.error(__FUNCTION__, "controller pipe created failed", pipe_in_ret, pipe_out_ret);
    }

    m_support_socket = true;
}

asst::ControllerPosix::~ControllerPosix()
{
    LogTraceFunction;

    release_minitouch();

    ::close(m_pipe_in[PIPE_READ]);
    ::close(m_pipe_in[PIPE_WRITE]);
    ::close(m_pipe_out[PIPE_READ]);
    ::close(m_pipe_out[PIPE_WRITE]);
}

std::optional<int> asst::ControllerPosix::call_command(const std::string& cmd, const bool recv_by_socket,
                                                        std::string& pipe_data, std::string& sock_data,
                                                        const int64_t timeout,
                                                        const std::chrono::steady_clock::time_point start_time)
{
    using namespace std::chrono;

    asst::platform::single_page_buffer<char> pipe_buffer;
    asst::platform::single_page_buffer<char> sock_buffer;

    auto check_timeout = [&]() -> bool {
        return timeout && timeout < duration_cast<milliseconds>(steady_clock::now() - start_time).count();
    };

    int exit_ret = 0;
    m_child = ::fork();
    if (m_child == 0) {
        // child process

        ::dup2(m_pipe_in[PIPE_READ], STDIN_FILENO);
        ::dup2(m_pipe_out[PIPE_WRITE], STDOUT_FILENO);
        ::dup2(m_pipe_out[PIPE_WRITE], STDERR_FILENO);

        // all these are for use by parent only
        // close(m_pipe_in[PIPE_READ]);
        // close(m_pipe_in[PIPE_WRITE]);
        // close(m_pipe_out[PIPE_READ]);
        // close(m_pipe_out[PIPE_WRITE]);

        exit_ret = execlp("sh", "sh", "-c", cmd.c_str(), nullptr);
        ::exit(exit_ret);
    }
    else if (m_child > 0) {
        // parent process
        if (recv_by_socket) {
            sockaddr addr {};
            socklen_t len = sizeof(addr);
            sock_buffer = asst::platform::single_page_buffer<char>();

            int client_socket = ::accept(m_server_sock, &addr, &len);
            if (client_socket < 0) {
                Log.error("accept failed:", strerror(errno));
                ::kill(m_child, SIGKILL);
                ::waitpid(m_child, &exit_ret, 0);
                return std::nullopt;
            }

            ssize_t read_num = ::read(client_socket, sock_buffer.get(), sock_buffer.size());
            while (read_num > 0) {
                sock_data.insert(sock_data.end(), sock_buffer.get(), sock_buffer.get() + read_num);
                read_num = ::read(client_socket, sock_buffer.get(), sock_buffer.size());
            }
            ::shutdown(client_socket, SHUT_RDWR);
            ::close(client_socket);
        }
        else {
            do {
                ssize_t read_num = ::read(m_pipe_out[PIPE_READ], pipe_buffer.get(), pipe_buffer.size());
                while (read_num > 0) {
                    pipe_data.insert(pipe_data.end(), pipe_buffer.get(), pipe_buffer.get() + read_num);
                    read_num = ::read(m_pipe_out[PIPE_READ], pipe_buffer.get(), pipe_buffer.size());
                }
            } while (::waitpid(m_child, &exit_ret, WNOHANG) == 0 && !check_timeout());
        }
        ::waitpid(m_child, &exit_ret, 0); // if ::waitpid(m_child, &exit_ret, WNOHANG) == 0, repeat it will cause
                                          // ECHILD, so not check the return value
    }
    else {
        // failed to create child process
        Log.error("Call `", cmd, "` create process failed, child:", m_child);
        return std::nullopt;
    }

    return exit_ret;
}

std::optional<unsigned short> asst::ControllerPosix::init_socket(const std::string& local_address)
{
    LogTraceFunction;

    m_server_sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_sock < 0) {
        return std::nullopt;
    }

    m_server_sock_addr.sin_family = AF_INET;
    m_server_sock_addr.sin_addr.s_addr = INADDR_ANY;

    bool server_start = false;
    uint16_t port_result = 0;

    m_server_sock_addr.sin_port = htons(0);
    int bind_ret = ::bind(m_server_sock, reinterpret_cast<sockaddr*>(&m_server_sock_addr), sizeof(::sockaddr_in));
    socklen_t addrlen = sizeof(m_server_sock_addr);
    int getname_ret = ::getsockname(m_server_sock, reinterpret_cast<sockaddr*>(&m_server_sock_addr), &addrlen);
    int listen_ret = ::listen(m_server_sock, 3);
    struct timeval timeout = { 6, 0 };
    int timeout_ret = ::setsockopt(m_server_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));
    server_start = bind_ret == 0 && getname_ret == 0 && listen_ret == 0 && timeout_ret == 0;

    if (!server_start) {
        Log.info("not supports socket");
        return std::nullopt;
    }

    port_result = ntohs(m_server_sock_addr.sin_port);

    Log.info("command server start", local_address, port_result);
    return port_result;
}

void asst::ControllerPosix::close_socket() noexcept
{
    if (m_server_sock >= 0) {
        ::close(m_server_sock);
        m_server_sock = -1;
    }
}

bool asst::ControllerPosix::call_and_hup_minitouch(const std::string& cmd, const int timeout, std::string& pipe_str)
{
    constexpr int PipeReadBuffSize = 4096ULL;
    constexpr int PipeWriteBuffSize = 64 * 1024ULL;

    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < std::chrono::seconds(timeout);
    };

    std::ignore = PipeWriteBuffSize;

    int pipe_to_child[2];
    int pipe_from_child[2];

    if (::pipe(pipe_to_child)) return false;
    if (::pipe(pipe_from_child)) {
        ::close(pipe_to_child[0]);
        ::close(pipe_to_child[1]);
        return false;
    }

    ::pid_t pid = fork();
    if (pid < 0) {
        Log.error("failed to create process");
        return false;
    }
    if (pid == 0) { // child process
        if (::dup2(pipe_to_child[0], STDIN_FILENO) < 0 || ::close(pipe_to_child[1]) < 0 ||
            ::close(pipe_from_child[0]) < 0 || ::dup2(pipe_from_child[1], STDOUT_FILENO) < 0 ||
            ::dup2(pipe_from_child[1], STDERR_FILENO) < 0) {
            ::exit(-1);
        }

        // set stdin of child to blocking
        if (int val = ::fcntl(STDIN_FILENO, F_GETFL); val != -1 && (val & O_NONBLOCK)) {
            val &= ~O_NONBLOCK;
            ::fcntl(STDIN_FILENO, F_SETFL, val);
        }

#ifndef __APPLE__
        ::prctl(PR_SET_PDEATHSIG, SIGTERM);
#endif

        ::execlp("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        exit(-1);
    }

    m_minitouch_process = pid;
    m_write_to_minitouch_fd = pipe_to_child[1];

    if (::close(pipe_from_child[1]) < 0 || ::close(pipe_to_child[0]) < 0) {
        release_minitouch(true);
        return false;
    }

    // set stdout to non blocking
    if (int val = ::fcntl(pipe_from_child[0], F_GETFL); val != -1) {
        val |= O_NONBLOCK;
        ::fcntl(pipe_from_child[0], F_SETFL, val);
    }
    else {
        release_minitouch(true);
        return false;
    }
    const auto start_time = std::chrono::steady_clock::now();

    while (true) {
        if (need_exit()) {
            release_minitouch(true);
            return false;
        }
        if (!check_timeout(start_time)) {
            Log.info("unable to find $ from pipe_str:", Logger::separator::newline, pipe_str);
            release_minitouch(true);
            return false;
        }

        if (pipe_str.find('$') != std::string::npos) {
            break;
        }

        char buf_from_child[PipeReadBuffSize];
        ssize_t ret = ::read(pipe_from_child[0], buf_from_child, PipeReadBuffSize);
        if (ret <= 0) continue;
        pipe_str.insert(pipe_str.end(), buf_from_child, buf_from_child + ret);
    }

    ::dup2(::open("/dev/null", O_WRONLY), pipe_from_child[0]);
    return true;
}

bool asst::ControllerPosix::input_to_minitouch(const std::string& cmd)
{
    if (m_minitouch_process < 0 || m_write_to_minitouch_fd < 0) return false;
    if (::write(m_write_to_minitouch_fd, cmd.c_str(), cmd.length()) >= 0) return true;
    Log.error("Failed to write to minitouch, err", errno);
    return false;
}

void asst::ControllerPosix::release_minitouch(bool force)
{
    if (m_write_to_minitouch_fd != -1) {
        ::close(m_write_to_minitouch_fd);
        m_write_to_minitouch_fd = -1;
    }
    if (m_minitouch_process > 0) {
        ::kill(m_minitouch_process, SIGTERM);
        if (force) ::kill(m_minitouch_process, SIGKILL);
        m_minitouch_process = -1;
    }
}

void asst::ControllerPosix::release_adb(const std::string& adb_release, int64_t timeout)
{
    if (m_child) {
        std::string pipe_data;
        std::string sock_data;
        auto start_time = std::chrono::steady_clock::now();

        call_command(adb_release, false, pipe_data, sock_data, timeout, start_time);
    }
}
#endif
