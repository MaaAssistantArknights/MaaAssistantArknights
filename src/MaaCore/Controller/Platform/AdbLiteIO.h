#pragma once

#include "PlatformIO.h"

#ifdef _WIN32
#include "Win32IO.h"
#else
#include "PosixIO.h"
#endif

#include "../adb-lite/client.hpp"

#include "Utils/Logger.hpp"

namespace asst
{
#ifdef _WIN32
using NativeIO = asst::Win32IO;
#else
using NativeIO = asst::PosixIO;
#endif
class AdbLiteIO : public NativeIO
{
public:
    AdbLiteIO(Assistant* inst) :
        NativeIO(inst) {};
    AdbLiteIO(const AdbLiteIO&) = delete;
    AdbLiteIO(AdbLiteIO&&) = delete;
    virtual ~AdbLiteIO() = default;

    virtual std::optional<int> call_command(
        const std::string& cmd,
        bool recv_by_socket,
        std::string& pipe_data,
        std::string& sock_data,
        int64_t timeout,
        std::chrono::steady_clock::time_point start_time) override;

    virtual std::shared_ptr<IOHandler> interactive_shell(const std::string& cmd) override;

    virtual void release_adb(const std::string& adb_release, int64_t timeout = 20000) override;

private:
    static bool remove_quotes(std::string& data);

    std::shared_ptr<adb::client> m_adb_client = nullptr;
};

class IOHandlerAdbLite : public IOHandler
{
public:
    IOHandlerAdbLite(std::shared_ptr<adb::io_handle> handle) :
        m_handle(handle) {};
    IOHandlerAdbLite(const IOHandlerAdbLite&) = delete;
    IOHandlerAdbLite(IOHandlerAdbLite&&) = delete;
    virtual ~IOHandlerAdbLite() = default;

    virtual bool write(const std::string_view data) override;
    virtual std::string read(unsigned timeout_sec) override;

private:
    std::shared_ptr<adb::io_handle> m_handle = nullptr;
};
} // namespace asst
