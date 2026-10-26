#include "socket_compat.h"

#include <algorithm>
#include <limits>
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <mutex>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace arps {
namespace socket_compat {
namespace {

constexpr int kMaxIoChunk = std::numeric_limits<int>::max();

#ifdef _WIN32

SOCKET ToNative(ArpsSocket socket) {
    return static_cast<SOCKET>(socket);
}

ArpsSocket FromNative(SOCKET socket) {
    return static_cast<ArpsSocket>(socket);
}

std::string LastSocketErrorMessage(const char* prefix) {
    int code = WSAGetLastError();
    char* buffer = nullptr;
    DWORD len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, static_cast<DWORD>(code),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<char*>(&buffer), 0,
            nullptr);
    std::string message;
    if (len > 0 && buffer != nullptr) {
        message.assign(buffer, len);
        while (!message.empty()
                && (message.back() == '\r' || message.back() == '\n'
                        || message.back() == ' ')) {
            message.pop_back();
        }
        LocalFree(buffer);
    } else {
        message = "Winsock error " + std::to_string(code);
    }

    std::ostringstream out;
    out << prefix << ": " << message;
    return out.str();
}

bool IsInterruptedSocketError() {
    return WSAGetLastError() == WSAEINTR;
}

#else

int ToNative(ArpsSocket socket) {
    return socket;
}

ArpsSocket FromNative(int socket) {
    return socket;
}

std::string LastSocketErrorMessage(const char* prefix) {
    std::ostringstream out;
    out << prefix << ": " << std::strerror(errno);
    return out.str();
}

bool IsInterruptedSocketError() {
    return errno == EINTR;
}

#endif

bool SetSocketOption(ArpsSocket socket, int level, int name, int value) {
#ifdef _WIN32
    const char* opt = reinterpret_cast<const char*>(&value);
#else
    const void* opt = &value;
#endif
    return setsockopt(ToNative(socket), level, name, opt,
            static_cast<int>(sizeof(value))) == 0;
}

#ifdef _WIN32
void CloseMany(ArpsSocket a, ArpsSocket b = kInvalidArpsSocket,
        ArpsSocket c = kInvalidArpsSocket) {
    Close(a);
    Close(b);
    Close(c);
}
#endif

}  // namespace

bool IsInvalid(ArpsSocket socket) {
    return socket == kInvalidArpsSocket;
}

bool EnsureRuntime(std::string* error) {
#ifdef _WIN32
    static std::once_flag startup_once;
    static int startup_result = 0;
    std::call_once(startup_once, [] {
        WSADATA data{};
        startup_result = WSAStartup(MAKEWORD(2, 2), &data);
    });
    if (startup_result != 0) {
        if (error) {
            std::ostringstream out;
            out << "WSAStartup: Winsock error " << startup_result;
            *error = out.str();
        }
        return false;
    }
#else
    (void)error;
#endif
    return true;
}

ArpsSocket OpenTcpSocket(std::string* error) {
    if (!EnsureRuntime(error)) {
        return kInvalidArpsSocket;
    }
#ifdef _WIN32
    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == INVALID_SOCKET) {
#else
    int socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0) {
#endif
        if (error) {
            *error = LastSocketErrorMessage("socket");
        }
        return kInvalidArpsSocket;
    }
    return FromNative(socket);
}

void Close(ArpsSocket socket) {
    if (IsInvalid(socket)) {
        return;
    }
#ifdef _WIN32
    closesocket(ToNative(socket));
#else
    close(ToNative(socket));
#endif
}

void SetReuseAddr(ArpsSocket socket) {
    SetSocketOption(socket, SOL_SOCKET, SO_REUSEADDR, 1);
}

void SetTcpNoDelay(ArpsSocket socket) {
    SetSocketOption(socket, IPPROTO_TCP, TCP_NODELAY, 1);
}

void SetNoSigpipe(ArpsSocket socket) {
#ifdef SO_NOSIGPIPE
    SetSocketOption(socket, SOL_SOCKET, SO_NOSIGPIPE, 1);
#else
    (void)socket;
#endif
}

bool BindIpv4(ArpsSocket socket, const std::string& host, std::uint16_t port,
        std::string* error) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        if (error) {
            *error = "invalid IPv4 listen host: " + host;
        }
        return false;
    }
    if (bind(ToNative(socket), reinterpret_cast<sockaddr*>(&addr),
                static_cast<int>(sizeof(addr))) != 0) {
        if (error) {
            *error = LastSocketErrorMessage("bind");
        }
        return false;
    }
    return true;
}

bool StartListening(ArpsSocket socket, int backlog, std::string* error) {
    if (listen(ToNative(socket), backlog) != 0) {
        if (error) {
            *error = LastSocketErrorMessage("listen");
        }
        return false;
    }
    return true;
}

ArpsSocket Accept(ArpsSocket socket, std::string* error) {
#ifdef _WIN32
    SOCKET accepted = accept(ToNative(socket), nullptr, nullptr);
    if (accepted == INVALID_SOCKET) {
#else
    int accepted = accept(ToNative(socket), nullptr, nullptr);
    if (accepted < 0) {
#endif
        if (error) {
            *error = LastSocketErrorMessage("accept");
        }
        return kInvalidArpsSocket;
    }
    return FromNative(accepted);
}

ArpsSocket ConnectIpv4(const std::string& host, std::uint16_t port, std::string* error) {
    ArpsSocket socket = OpenTcpSocket(error);
    if (IsInvalid(socket)) {
        return kInvalidArpsSocket;
    }
    SetTcpNoDelay(socket);
    SetNoSigpipe(socket);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        if (error) {
            *error = "invalid IPv4 host: " + host;
        }
        Close(socket);
        return kInvalidArpsSocket;
    }
    if (connect(ToNative(socket), reinterpret_cast<sockaddr*>(&addr),
                static_cast<int>(sizeof(addr))) != 0) {
        if (error) {
            *error = LastSocketErrorMessage("connect");
        }
        Close(socket);
        return kInvalidArpsSocket;
    }
    return socket;
}

WaitStatus WaitReadable(ArpsSocket socket, int timeout_ms, const char* operation,
        std::string* error) {
#ifdef _WIN32
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(ToNative(socket), &readfds);

    timeval timeout{};
    timeval* timeout_ptr = nullptr;
    if (timeout_ms >= 0) {
        timeout.tv_sec = static_cast<long>(timeout_ms / 1000);
        timeout.tv_usec = static_cast<long>((timeout_ms % 1000) * 1000);
        timeout_ptr = &timeout;
    }

    int rc = select(0, &readfds, nullptr, nullptr, timeout_ptr);
    if (rc == 0) {
        return WaitStatus::Timeout;
    }
    if (rc == SOCKET_ERROR) {
        if (error) {
            *error = LastSocketErrorMessage(operation);
        }
        return WaitStatus::Error;
    }
    return WaitStatus::Ready;
#else
    pollfd pfd{};
    pfd.fd = ToNative(socket);
    pfd.events = POLLIN;
    while (true) {
        int rc = poll(&pfd, 1, timeout_ms < 0 ? -1 : timeout_ms);
        if (rc == 0) {
            return WaitStatus::Timeout;
        }
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (error) {
                *error = LastSocketErrorMessage(operation);
            }
            return WaitStatus::Error;
        }
        if (pfd.revents & (POLLERR | POLLNVAL)) {
            if (error) {
                *error = "socket read failed";
            }
            return WaitStatus::Error;
        }
        return WaitStatus::Ready;
    }
#endif
}

int Recv(ArpsSocket socket, std::uint8_t* data, std::size_t len, std::string* error) {
    std::size_t chunk = std::min(len, static_cast<std::size_t>(kMaxIoChunk));
    while (true) {
#ifdef _WIN32
        int got = recv(ToNative(socket), reinterpret_cast<char*>(data),
                static_cast<int>(chunk), 0);
        if (got == SOCKET_ERROR) {
#else
        ssize_t got = recv(ToNative(socket), data, chunk, 0);
        if (got < 0) {
#endif
            if (IsInterruptedSocketError()) {
                continue;
            }
            if (error) {
                *error = LastSocketErrorMessage("recv");
            }
            return -1;
        }
        return static_cast<int>(got);
    }
}

int Send(ArpsSocket socket, const std::uint8_t* data, std::size_t len, std::string* error) {
    std::size_t chunk = std::min(len, static_cast<std::size_t>(kMaxIoChunk));
    while (true) {
#ifdef _WIN32
        int wrote = send(ToNative(socket), reinterpret_cast<const char*>(data),
                static_cast<int>(chunk), 0);
        if (wrote == SOCKET_ERROR) {
#else
        int flags = 0;
#ifdef MSG_NOSIGNAL
        flags = MSG_NOSIGNAL;
#endif
        ssize_t wrote = send(ToNative(socket), data, chunk, flags);
        if (wrote < 0) {
#endif
            if (IsInterruptedSocketError()) {
                continue;
            }
            if (error) {
                *error = LastSocketErrorMessage("send");
            }
            return -1;
        }
        return static_cast<int>(wrote);
    }
}

bool CreateConnectedSocketPair(ArpsSocket sockets[2], std::string* error) {
    sockets[0] = kInvalidArpsSocket;
    sockets[1] = kInvalidArpsSocket;
    if (!EnsureRuntime(error)) {
        return false;
    }

#ifndef _WIN32
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
        if (error) {
            *error = LastSocketErrorMessage("socketpair");
        }
        return false;
    }
    sockets[0] = FromNative(fds[0]);
    sockets[1] = FromNative(fds[1]);
    return true;
#else
    ArpsSocket listener = OpenTcpSocket(error);
    if (IsInvalid(listener)) {
        return false;
    }
    SetReuseAddr(listener);
    if (!BindIpv4(listener, "127.0.0.1", 0, error)) {
        Close(listener);
        return false;
    }
    if (!StartListening(listener, 1, error)) {
        Close(listener);
        return false;
    }

    sockaddr_in bound{};
    int bound_len = static_cast<int>(sizeof(bound));
    if (getsockname(ToNative(listener), reinterpret_cast<sockaddr*>(&bound), &bound_len)
            != 0) {
        if (error) {
            *error = LastSocketErrorMessage("getsockname");
        }
        Close(listener);
        return false;
    }

    ArpsSocket client = OpenTcpSocket(error);
    if (IsInvalid(client)) {
        Close(listener);
        return false;
    }
    if (connect(ToNative(client), reinterpret_cast<sockaddr*>(&bound),
                static_cast<int>(sizeof(bound))) != 0) {
        if (error) {
            *error = LastSocketErrorMessage("connect");
        }
        CloseMany(listener, client);
        return false;
    }

    ArpsSocket server = Accept(listener, error);
    Close(listener);
    if (IsInvalid(server)) {
        Close(client);
        return false;
    }

    sockets[0] = server;
    sockets[1] = client;
    return true;
#endif
}

}  // namespace socket_compat
}  // namespace arps
