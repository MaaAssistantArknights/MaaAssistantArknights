#ifndef _WIN32
#include "PosixIO.h"

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

asst::PosixIO::PosixIO(Assistant* inst) : InstHelper(inst)
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

asst::PosixIO::~PosixIO()
{
    LogTraceFunction;

    if (m_server_sock >= 0) {
        ::close(m_server_sock);
        m_server_sock = -1;
    }

    ::close(m_pipe_in[PIPE_READ]);
    ::close(m_pipe_in[PIPE_WRITE]);
    ::close(m_pipe_out[PIPE_READ]);
    ::close(m_pipe_out[PIPE_WRITE]);
}

std::optional<int> asst::PosixIO::call_command(const std::string& cmd, const bool recv_by_socket,
                                               std::string& pipe_data, std::string& sock_data, const int64_t timeout,
                                               std::chrono::steady_clock::time_point start_time)
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

std::optional<unsigned short> asst::PosixIO::init_socket(const std::string& local_address)
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

void asst::PosixIO::close_socket() noexcept
{
    if (m_server_sock >= 0) {
        ::close(m_server_sock);
        m_server_sock = -1;
    }
}

std::shared_ptr<asst::IOHandler> asst::PosixIO::interactive_shell(const std::string& cmd)
{
    LogTraceFunction;

    int pipe_to_child[2];
    int pipe_from_child[2];

    if (::pipe(pipe_to_child)) return nullptr;
    if (::pipe(pipe_from_child)) {
        ::close(pipe_to_child[0]);
        ::close(pipe_to_child[1]);
        return nullptr;
    }

    ::pid_t pid = fork();
    if (pid < 0) {
        ::close(pipe_to_child[0]);
        ::close(pipe_to_child[1]);
        ::close(pipe_from_child[0]);
        ::close(pipe_from_child[1]);
        Log.error("fork failed:", strerror(errno));
        return nullptr;
    }
    if (pid == 0) {
        // child process
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

    if (::close(pipe_to_child[0]) < 0 || ::close(pipe_from_child[1]) < 0) {
        ::kill(pid, SIGTERM);
        ::waitpid(pid, nullptr, 0);
        ::close(pipe_to_child[1]);
        ::close(pipe_from_child[0]);
        return nullptr;
    }

    // set stdout to non blocking
    if (int val = ::fcntl(pipe_from_child[0], F_GETFL); val != -1) {
        val |= O_NONBLOCK;
        ::fcntl(pipe_from_child[0], F_SETFL, val);
    }
    else {
        ::kill(pid, SIGTERM);
        ::waitpid(pid, nullptr, 0);
        ::close(pipe_to_child[1]);
        ::close(pipe_from_child[0]);
        return nullptr;
    }

    return std::make_shared<IOHandlerPosix>(pipe_from_child[0], pipe_to_child[1], pid);
}

void asst::PosixIO::release_adb(const std::string& adb_release, int64_t timeout)
{
    if (m_child) {
        std::string pipe_data;
        std::string sock_data;
        auto start_time = std::chrono::steady_clock::now();

        call_command(adb_release, false, pipe_data, sock_data, timeout, start_time);
    }
}

asst::IOHandlerPosix::~IOHandlerPosix()
{
    if (m_write_fd != -1) ::close(m_write_fd);
    if (m_read_fd != -1) ::close(m_read_fd);
    if (m_process > 0) ::kill(m_process, SIGTERM);
}

bool asst::IOHandlerPosix::write(std::string_view data)
{
    if (m_process < 0 || m_write_fd < 0) return false;
    if (::write(m_write_fd, data.data(), data.length()) >= 0) return true;
    Log.error("Failed to write to IOHandlerPosix, err", errno);
    return false;
}

std::string asst::IOHandlerPosix::read(unsigned timeout_sec)
{
    if (m_process < 0 || m_read_fd < 0) return {};
    std::string ret_str;
    constexpr int PipeReadBuffSize = 4096ULL;

    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < timeout_sec * 1s;
    };

    auto start_time = std::chrono::steady_clock::now();

    while (true) {
        char buf_from_child[PipeReadBuffSize];

        if (!check_timeout(start_time)) {
            break;
        }

        ssize_t ret_read = ::read(m_read_fd, buf_from_child, PipeReadBuffSize);
        if (ret_read > 0) {
            ret_str.insert(ret_str.end(), buf_from_child, buf_from_child + ret_read);
        }
        else {
            break;
        }
    }
    return ret_str;
}
#endif
