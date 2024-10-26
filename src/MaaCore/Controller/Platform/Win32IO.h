#pragma once

#ifdef _WIN32
#include "PlatformIO.h"

#include "Utils/Platform/SafeWindows.h"
#include <mswsock.h>

#include "Common/AsstTypes.h"
#include "InstHelper.h"
#include "Utils/SingletonHolder.hpp"

namespace asst
{
class Win32IO : public PlatformIO, private InstHelper
{
public:
    Win32IO(Assistant* inst);
    Win32IO(const Win32IO&) = delete;
    Win32IO(Win32IO&&) = delete;
    virtual ~Win32IO();

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

    virtual void release_adb(const std::string& adb_release, int64_t timeout = 20000);

    ASST_AUTO_DEDUCED_ZERO_INIT_START
    WSADATA m_wsa_data {};
    SOCKET m_server_sock = INVALID_SOCKET;
    sockaddr_in m_server_sock_addr {};
    LPFN_ACCEPTEX m_server_accept_ex = nullptr;
    ASST_AUTO_DEDUCED_ZERO_INIT_END

private:
    // for Windows socket
    class WsaHelper : public SingletonHolder<WsaHelper>
    {
        friend class SingletonHolder<WsaHelper>;

    public:
        virtual ~WsaHelper() override { WSACleanup(); }

        bool operator()() const noexcept { return m_supports; }

    private:
        WsaHelper()
        {
            m_supports = WSAStartup(MAKEWORD(2, 2), &m_wsa_data) == 0 && m_wsa_data.wVersion == MAKEWORD(2, 2);
        }

        WSADATA m_wsa_data = { 0 };
        bool m_supports = false;
    };
};

class IOHandlerWin32 : public IOHandler
{
public:
    IOHandlerWin32(HANDLE read, HANDLE write, PROCESS_INFORMATION process_info) :
        m_read(read),
        m_write(write),
        m_process_info(process_info)
    {
    }

    IOHandlerWin32(const IOHandlerWin32&) = delete;
    IOHandlerWin32(IOHandlerWin32&&) = delete;
    virtual ~IOHandlerWin32();

    virtual bool write(std::string_view data) override;
    virtual std::string read(unsigned timeout_sec) override;

private:
    HANDLE m_read = INVALID_HANDLE_VALUE;
    HANDLE m_write = INVALID_HANDLE_VALUE;
    PROCESS_INFORMATION m_process_info = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 };

    const int PipeBufferSize = 4096;
};
} // namespace asst
#endif
