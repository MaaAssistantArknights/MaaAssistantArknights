#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
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

struct ArpsPowerControlOptions {
    std::optional<std::uint32_t> display_id;
    std::string request_id;
    std::string reason;
    std::optional<bool> keep_screen_on;
    std::optional<bool> power_on_if_screen_off;
    std::optional<std::string> screen_interactive;
    std::optional<std::string> display_power;

    std::string ToJson() const;
};

struct ArpsPowerState {
    std::string request_id;
    bool ok = false;
    std::string error;
    std::string reason;
    std::uint32_t display_id = 0;
    bool screen_on = false;
    bool previous_screen_on = false;
    bool wake_lock_held_by_arps = false;
    std::string display_power_override = "unknown";
};

enum class ArpsReadStatus {
    Frame,
    Hello,
    Ready,
    PowerState,
    Error,
    Stop,
    Timeout,
    Closed,
    ProtocolError,
};

struct ArpsReadResult {
    ArpsReadStatus status = ArpsReadStatus::Closed;
    ArpsFrame frame;
    ArpsPowerState power_state;
    std::string json;
    std::string message;
    std::uint16_t packet_type = 0;
    std::uint16_t protocol_major = 0;
    std::uint16_t protocol_minor = 0;
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
    bool RequestFrame(std::string* error);
    bool SendPowerControl(const ArpsPowerControlOptions& options, std::string* error);
    bool RequestPowerState(const std::string& request_id, std::string* error);
    bool SendPowerControl(bool keep_screen_on, bool power_on_if_screen_off,
            const std::string& reason, std::string* error);
    bool SendStop(const std::string& reason, std::string* error);
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
