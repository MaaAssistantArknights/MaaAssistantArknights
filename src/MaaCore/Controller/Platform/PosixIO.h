#pragma once

#ifndef _WIN32
#include <netinet/in.h>

#include "PlatformIO.h"

#include "InstHelper.h"

namespace asst
{
class PosixIO : public PlatformIO, private InstHelper
{
public:
    PosixIO(Assistant* inst);
    PosixIO(const PosixIO&) = delete;
    PosixIO(PosixIO&&) = delete;
    virtual ~PosixIO();

    virtual std::optional<int> call_command(
        const std::string& cmd,
        bool recv_by_socket,
        std::string& pipe_data,
        std::string& sock_data,
        int64_t timeout,
        std::chrono::steady_clock::time_point start_time) override;

    virtual std::optional<unsigned short> init_socket(const std::string& local_address) override;
    virtual void close_socket() noexcept override;

    virtual std::shared_ptr<IOHandler> interactive_shell(const std::string& cmd) override;

    virtual void release_adb(const std::string& adb_release, int64_t timeout = 20000) override;

    int m_server_sock = -1;
    sockaddr_in m_server_sock_addr {};
    static constexpr int PIPE_READ = 0;
    static constexpr int PIPE_WRITE = 1;
    int m_pipe_in[2] = { 0 };
    int m_pipe_out[2] = { 0 };
    int m_child = 0;
};

class IOHandlerPosix : public IOHandler
{
public:
    IOHandlerPosix(int read_fd, int write_fd, ::pid_t process)
    {
        m_read_fd = read_fd;
        m_write_fd = write_fd;
        m_process = process;
    }

    IOHandlerPosix(const IOHandlerPosix&) = delete;
    IOHandlerPosix(IOHandlerPosix&&) = delete;
    virtual ~IOHandlerPosix() override;

    virtual bool write(std::string_view data) override;
    virtual std::string read(unsigned timeout_sec) override;

private:
    int m_read_fd = -1;
    int m_write_fd = -1;
    ::pid_t m_process = -1;
};
}
#endif
