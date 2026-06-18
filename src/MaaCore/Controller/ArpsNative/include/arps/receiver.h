#pragma once

#include <cstdint>
#include <mutex>
#include <string>

#include "arps/frame.h"
#include "arps/protocol.h"
#include "arps/socket.h"

namespace arps {

struct ArpsStartOptions {
    std::uint32_t display_id = 0;
    std::string pixel_format = "argb8888";
    std::string compression = "lz4_block";
    std::uint32_t max_fps = 30;
    std::uint32_t max_packet_len = kDefaultMaxPacketLen;
    bool power_on_if_screen_off = true;
    bool turn_screen_off = false;
    bool keep_screen_on = true;
    std::string capture_mode = "auto";
    std::string exit_power_mode = "restore_previous";
    std::string stream_mode = "push";

    std::string ToJson() const;
};

enum class ArpsReadStatus {
    Frame,
    Hello,
    Ready,
    Error,
    Stop,
    Timeout,
    Closed,
    ProtocolError,
};

struct ArpsReadResult {
    ArpsReadStatus status = ArpsReadStatus::Closed;
    ArpsFrame frame;
    std::string json;
    std::string message;
    std::uint16_t packet_type = 0;
    std::uint32_t sequence = 0;
};

class ArpsReceiver {
public:
    ArpsReceiver();
    ~ArpsReceiver();

    ArpsReceiver(const ArpsReceiver&) = delete;
    ArpsReceiver& operator=(const ArpsReceiver&) = delete;

    bool Listen(const std::string& host, std::uint16_t port, std::string* error);
    bool AcceptOnce(int timeout_ms, std::string* error);
    bool AdoptConnectedSocket(ArpsSocket socket, std::string* error);
    bool SendStart(const ArpsStartOptions& options, std::string* error);
    bool SendStop(std::string* error);
    bool RequestFrame(std::string* error);
    ArpsReadResult ReadNext(int timeout_ms);
    void Close();

    ArpsSocket client_socket() const { return client_socket_; }
    ArpsSocket client_fd() const { return client_socket_; }

private:
    ArpsSocket listen_socket_ = kInvalidArpsSocket;
    ArpsSocket client_socket_ = kInvalidArpsSocket;
    std::uint32_t max_packet_len_ = kDefaultMaxPacketLen;
    std::uint32_t next_sequence_ = 1;
    std::mutex write_mutex_;

    bool SendControlPacket(std::uint16_t type, const std::string& ext, std::string* error);

    class Impl;
    Impl* impl_ = nullptr;
};

}  // namespace arps
