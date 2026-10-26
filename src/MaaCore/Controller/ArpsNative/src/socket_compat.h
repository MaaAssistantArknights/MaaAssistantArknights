#pragma once

#include "arps/socket.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace arps {
namespace socket_compat {

enum class WaitStatus {
    Ready,
    Timeout,
    Error,
};

bool IsInvalid(ArpsSocket socket);
bool EnsureRuntime(std::string* error);
ArpsSocket OpenTcpSocket(std::string* error);
void Close(ArpsSocket socket);
void SetReuseAddr(ArpsSocket socket);
void SetTcpNoDelay(ArpsSocket socket);
void SetNoSigpipe(ArpsSocket socket);
bool BindIpv4(ArpsSocket socket, const std::string& host, std::uint16_t port,
        std::string* error);
bool StartListening(ArpsSocket socket, int backlog, std::string* error);
ArpsSocket Accept(ArpsSocket socket, std::string* error);
ArpsSocket ConnectIpv4(const std::string& host, std::uint16_t port, std::string* error);
WaitStatus WaitReadable(ArpsSocket socket, int timeout_ms, const char* operation,
        std::string* error);
int Recv(ArpsSocket socket, std::uint8_t* data, std::size_t len, std::string* error);
int Send(ArpsSocket socket, const std::uint8_t* data, std::size_t len,
        std::string* error);
bool CreateConnectedSocketPair(ArpsSocket sockets[2], std::string* error);

}  // namespace socket_compat
}  // namespace arps
