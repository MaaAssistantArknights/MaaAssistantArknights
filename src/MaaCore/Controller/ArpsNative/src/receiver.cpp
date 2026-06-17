#include "arps/receiver.h"

#include "socket_compat.h"

#include <lz4.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <sstream>
#include <utility>
#include <vector>

namespace arps {
namespace {

constexpr int kNoTimeout = -1;
namespace sockets = socket_compat;

struct RawPacket {
    std::uint16_t major = 0;
    std::uint16_t minor = 0;
    std::uint16_t type = 0;
    std::uint32_t flags = 0;
    std::uint32_t sequence = 0;
    std::uint32_t packet_len = 0;
    std::vector<std::uint8_t> base;
    std::vector<std::uint8_t> bitmap;
    std::string ext;
    double packet_read_ms = -1.0;
};

std::uint16_t ReadBe16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>((p[0] << 8) | p[1]);
}

std::uint32_t ReadBe32(const std::uint8_t* p) {
    return (static_cast<std::uint32_t>(p[0]) << 24)
            | (static_cast<std::uint32_t>(p[1]) << 16)
            | (static_cast<std::uint32_t>(p[2]) << 8)
            | static_cast<std::uint32_t>(p[3]);
}

std::uint64_t ReadBe64(const std::uint8_t* p) {
    return (static_cast<std::uint64_t>(ReadBe32(p)) << 32) | ReadBe32(p + 4);
}

void WriteBe16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
    out.push_back(static_cast<std::uint8_t>(value & 0xff));
}

void WriteBe32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xff));
    out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xff));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
    out.push_back(static_cast<std::uint8_t>(value & 0xff));
}

double MsSince(std::chrono::steady_clock::time_point start) {
    using Duration = std::chrono::duration<double, std::milli>;
    return Duration(std::chrono::steady_clock::now() - start).count();
}

int RemainingTimeoutMs(std::chrono::steady_clock::time_point deadline) {
    if (deadline == std::chrono::steady_clock::time_point::max()) {
        return kNoTimeout;
    }
    auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
    if (remaining.count() <= 0) {
        return 0;
    }
    if (remaining.count() > std::numeric_limits<int>::max()) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(remaining.count());
}

std::chrono::steady_clock::time_point DeadlineFromTimeout(int timeout_ms) {
    if (timeout_ms < 0) {
        return std::chrono::steady_clock::time_point::max();
    }
    return std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
}

enum class IoStatus {
    Ok,
    Timeout,
    Closed,
    Error,
};

IoStatus ReadExact(ArpsSocket socket, std::uint8_t* data, std::size_t len, int timeout_ms,
        std::string* error) {
    std::size_t offset = 0;
    auto deadline = DeadlineFromTimeout(timeout_ms);
    while (offset < len) {
        int timeout = offset == 0 ? RemainingTimeoutMs(deadline) : kNoTimeout;
        sockets::WaitStatus wait = sockets::WaitReadable(socket, timeout, "wait(read)", error);
        if (wait == sockets::WaitStatus::Timeout) {
            return IoStatus::Timeout;
        }
        if (wait == sockets::WaitStatus::Error) {
            return IoStatus::Error;
        }
        int got = sockets::Recv(socket, data + offset, len - offset, error);
        if (got == 0) {
            return IoStatus::Closed;
        }
        if (got < 0) {
            return IoStatus::Error;
        }
        offset += static_cast<std::size_t>(got);
    }
    return IoStatus::Ok;
}

IoStatus WriteExact(ArpsSocket socket, const std::uint8_t* data, std::size_t len,
        std::string* error) {
    std::size_t offset = 0;
    while (offset < len) {
        int wrote = sockets::Send(socket, data + offset, len - offset, error);
        if (wrote < 0) {
            return IoStatus::Error;
        }
        if (wrote == 0) {
            if (error) {
                *error = "send returned 0";
            }
            return IoStatus::Closed;
        }
        offset += static_cast<std::size_t>(wrote);
    }
    return IoStatus::Ok;
}

bool ReadU32SectionLength(ArpsSocket socket, std::uint32_t packet_len,
        std::uint32_t consumed,
        int timeout_ms, std::uint32_t* out, std::string* error, IoStatus* status) {
    if (packet_len - consumed < 4) {
        if (error) {
            *error = "packet section length missing";
        }
        return false;
    }
    std::uint8_t buf[4];
    *status = ReadExact(socket, buf, sizeof(buf), timeout_ms, error);
    if (*status != IoStatus::Ok) {
        return false;
    }
    std::uint32_t len = ReadBe32(buf);
    if (len > packet_len - consumed - 4) {
        if (error) {
            *error = "section length exceeds packet body";
        }
        return false;
    }
    *out = len;
    return true;
}

bool ReadBytes(ArpsSocket socket, std::uint32_t len, int timeout_ms,
        std::vector<std::uint8_t>* out, std::string* error, IoStatus* status) {
    out->assign(len, 0);
    if (len == 0) {
        *status = IoStatus::Ok;
        return true;
    }
    *status = ReadExact(socket, out->data(), out->size(), timeout_ms, error);
    return *status == IoStatus::Ok;
}

bool SkipBytes(ArpsSocket socket, std::uint32_t len, int timeout_ms, std::string* error,
        IoStatus* status) {
    std::uint8_t buffer[16 * 1024];
    std::uint32_t remaining = len;
    while (remaining > 0) {
        std::size_t chunk = std::min<std::uint32_t>(remaining, sizeof(buffer));
        *status = ReadExact(socket, buffer, chunk, timeout_ms, error);
        if (*status != IoStatus::Ok) {
            return false;
        }
        remaining -= static_cast<std::uint32_t>(chunk);
    }
    return true;
}

std::uint32_t Crc32(const std::uint8_t* data, std::size_t len) {
    std::uint32_t crc = 0xffffffffu;
    for (std::size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            std::uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1) ^ (0xedb88320u & mask);
        }
    }
    return ~crc;
}

double JsonNumber(const std::string& json, const char* key) {
    std::string quoted = std::string("\"") + key + "\"";
    std::size_t pos = json.find(quoted);
    if (pos == std::string::npos) {
        return -1.0;
    }
    pos = json.find(':', pos + quoted.size());
    if (pos == std::string::npos) {
        return -1.0;
    }
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) {
        ++pos;
    }
    char* end = nullptr;
    double value = std::strtod(json.c_str() + pos, &end);
    if (end == json.c_str() + pos || !std::isfinite(value)) {
        return -1.0;
    }
    return value;
}

ArpsDeviceTimings ParseTimings(const std::string& json) {
    ArpsDeviceTimings timings;
    timings.capture_ms = JsonNumber(json, "capture_ms");
    timings.copy_ms = JsonNumber(json, "copy_ms");
    timings.compress_ms = JsonNumber(json, "compress_ms");
    timings.write_ms = JsonNumber(json, "write_ms");
    timings.previous_write_ms = JsonNumber(json, "previous_write_ms");
    return timings;
}

ArpsFrameMeta ParseFrameMeta(const RawPacket& packet) {
    const std::uint8_t* p = packet.base.data();
    ArpsFrameMeta meta;
    meta.protocol_major = packet.major;
    meta.protocol_minor = packet.minor;
    meta.frame_no = ReadBe64(p);
    meta.monotonic_time_ns = ReadBe64(p + 8);
    meta.width = ReadBe32(p + 16);
    meta.height = ReadBe32(p + 20);
    meta.row_bytes = ReadBe32(p + 24);
    meta.rotation = ReadBe32(p + 28);
    meta.pixel_format = ReadBe32(p + 32);
    meta.compression_type = ReadBe32(p + 36);
    meta.uncompressed_len = ReadBe32(p + 40);
    meta.compressed_len = ReadBe32(p + 44);
    meta.display_id = ReadBe32(p + 48);
    meta.color_space = ReadBe32(p + 52);
    meta.payload_checksum = ReadBe32(p + 56);
    meta.flags = ReadBe32(p + 60);
    return meta;
}

ArpsReadResult ProtocolError(std::string message, const RawPacket* packet = nullptr) {
    ArpsReadResult result;
    result.status = ArpsReadStatus::ProtocolError;
    result.message = std::move(message);
    if (packet) {
        result.packet_type = packet->type;
        result.sequence = packet->sequence;
    }
    return result;
}

ArpsReadResult StatusResult(ArpsReadStatus status, const RawPacket& packet) {
    ArpsReadResult result;
    result.status = status;
    result.packet_type = packet.type;
    result.sequence = packet.sequence;
    result.json = packet.ext;
    return result;
}

}  // namespace

class ArpsReceiver::Impl {
public:
    std::vector<std::uint8_t> frame_buffer;

    ArpsReadResult ReadPacket(ArpsSocket socket, std::uint32_t max_packet_len,
            int timeout_ms) {
        RawPacket packet;
        std::string error;
        IoStatus status = IoStatus::Ok;
        auto start = std::chrono::steady_clock::now();

        std::uint8_t header[kHeaderLen];
        status = ReadExact(socket, header, sizeof(header), timeout_ms, &error);
        if (status == IoStatus::Timeout) {
            ArpsReadResult result;
            result.status = ArpsReadStatus::Timeout;
            result.message = "read timeout";
            return result;
        }
        if (status == IoStatus::Closed) {
            ArpsReadResult result;
            result.status = ArpsReadStatus::Closed;
            result.message = "socket closed";
            return result;
        }
        if (status == IoStatus::Error) {
            return ProtocolError(error.empty() ? "read header failed" : error);
        }

        if (std::memcmp(header, kMagic, kMagicSize) != 0) {
            return ProtocolError("bad ARPS magic");
        }
        packet.major = ReadBe16(header + 12);
        packet.minor = ReadBe16(header + 14);
        packet.type = ReadBe16(header + 16);
        std::uint16_t header_len = ReadBe16(header + 18);
        packet.flags = ReadBe32(header + 20);
        packet.sequence = ReadBe32(header + 24);
        packet.packet_len = ReadBe32(header + 28);

        if (packet.major != kProtocolMajor) {
            return ProtocolError("unsupported protocol_major", &packet);
        }
        if (header_len != kHeaderLen) {
            return ProtocolError("unsupported header_len", &packet);
        }
        if (packet.packet_len < 12 || packet.packet_len > max_packet_len) {
            return ProtocolError("invalid packet_len", &packet);
        }

        std::uint32_t consumed = 0;
        std::uint32_t base_len = 0;
        int body_timeout_ms = kNoTimeout;
        if (!ReadU32SectionLength(socket, packet.packet_len, consumed, body_timeout_ms,
                    &base_len, &error, &status)) {
            return PacketReadFailure(status, error, &packet);
        }
        consumed += 4;
        if (!ReadBytes(socket, base_len, body_timeout_ms, &packet.base, &error, &status)) {
            return PacketReadFailure(status, error, &packet);
        }
        consumed += base_len;

        std::uint32_t bitmap_len = 0;
        if (!ReadU32SectionLength(socket, packet.packet_len, consumed, body_timeout_ms,
                    &bitmap_len, &error, &status)) {
            return PacketReadFailure(status, error, &packet);
        }
        consumed += 4;
        if (!ReadBytes(socket, bitmap_len, body_timeout_ms, &packet.bitmap, &error,
                    &status)) {
            return PacketReadFailure(status, error, &packet);
        }
        consumed += bitmap_len;

        std::uint32_t ext_len = 0;
        if (!ReadU32SectionLength(socket, packet.packet_len, consumed, body_timeout_ms,
                    &ext_len, &error, &status)) {
            return PacketReadFailure(status, error, &packet);
        }
        consumed += 4;
        std::vector<std::uint8_t> ext_bytes;
        if (!ReadBytes(socket, ext_len, body_timeout_ms, &ext_bytes, &error, &status)) {
            return PacketReadFailure(status, error, &packet);
        }
        packet.ext.assign(reinterpret_cast<const char*>(ext_bytes.data()), ext_bytes.size());
        consumed += ext_len;

        if (consumed > packet.packet_len) {
            return ProtocolError("packet sections exceed packet_len", &packet);
        }
        std::uint32_t tail_len = packet.packet_len - consumed;
        if (!SkipBytes(socket, tail_len, body_timeout_ms, &error, &status)) {
            return PacketReadFailure(status, error, &packet);
        }
        packet.packet_read_ms = MsSince(start);

        switch (packet.type) {
            case kPacketHello:
                return StatusResult(ArpsReadStatus::Hello, packet);
            case kPacketReady:
                return StatusResult(ArpsReadStatus::Ready, packet);
            case kPacketError:
                return StatusResult(ArpsReadStatus::Error, packet);
            case kPacketStop:
                return StatusResult(ArpsReadStatus::Stop, packet);
            case kPacketFrame:
                return DecodeFrame(packet, max_packet_len);
            default:
                return ProtocolError("unknown packet_type", &packet);
        }
    }

private:
    ArpsReadResult PacketReadFailure(IoStatus status, const std::string& error,
            const RawPacket* packet) {
        if (status == IoStatus::Timeout) {
            ArpsReadResult result;
            result.status = ArpsReadStatus::Timeout;
            result.message = "read timeout";
            return result;
        }
        if (status == IoStatus::Closed) {
            ArpsReadResult result;
            result.status = ArpsReadStatus::Closed;
            result.message = "socket closed while reading packet";
            return result;
        }
        return ProtocolError(error.empty() ? "packet read failed" : error, packet);
    }

    ArpsReadResult DecodeFrame(const RawPacket& packet, std::uint32_t max_packet_len) {
        if (packet.base.size() < kFrameBaseLenV1) {
            return ProtocolError("FRAME base_len is smaller than BaseData v1", &packet);
        }
        ArpsFrameMeta meta = ParseFrameMeta(packet);
        if (meta.pixel_format != kPixelFormatAndroidArgb8888Raw) {
            return ProtocolError("unsupported pixel_format", &packet);
        }
        if (meta.compressed_len != packet.bitmap.size()) {
            return ProtocolError("compressed_len does not match bitmap_len", &packet);
        }
        if (meta.width == 0 || meta.height == 0) {
            return ProtocolError("invalid frame dimensions", &packet);
        }
        std::uint64_t min_row_bytes = static_cast<std::uint64_t>(meta.width) * 4u;
        if (meta.row_bytes < min_row_bytes) {
            return ProtocolError("row_bytes is smaller than width * 4", &packet);
        }
        std::uint64_t expected_len = static_cast<std::uint64_t>(meta.row_bytes) * meta.height;
        if (expected_len > std::numeric_limits<std::uint32_t>::max()
                || meta.uncompressed_len != expected_len) {
            return ProtocolError("uncompressed_len does not equal row_bytes * height", &packet);
        }
        if (meta.uncompressed_len > max_packet_len) {
            return ProtocolError("uncompressed_len exceeds max_packet_len", &packet);
        }
        if (meta.payload_checksum != 0
                && Crc32(packet.bitmap.data(), packet.bitmap.size()) != meta.payload_checksum) {
            return ProtocolError("payload_checksum mismatch", &packet);
        }

        auto decode_start = std::chrono::steady_clock::now();
        if (meta.compression_type == kCompressionRaw) {
            if (packet.bitmap.size() != meta.uncompressed_len) {
                return ProtocolError("raw bitmap_len does not equal uncompressed_len", &packet);
            }
            frame_buffer = packet.bitmap;
        } else if (meta.compression_type == kCompressionLz4Block) {
            if (packet.bitmap.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())
                    || meta.uncompressed_len > static_cast<std::uint32_t>(
                            std::numeric_limits<int>::max())) {
                return ProtocolError("LZ4 input or output exceeds decoder limits", &packet);
            }
            frame_buffer.assign(meta.uncompressed_len, 0);
            int decoded = LZ4_decompress_safe(
                    reinterpret_cast<const char*>(packet.bitmap.data()),
                    reinterpret_cast<char*>(frame_buffer.data()),
                    static_cast<int>(packet.bitmap.size()),
                    static_cast<int>(frame_buffer.size()));
            if (decoded < 0 || static_cast<std::uint32_t>(decoded) != meta.uncompressed_len) {
                return ProtocolError("LZ4 decompression failed or produced wrong length", &packet);
            }
        } else {
            return ProtocolError("unsupported compression_type", &packet);
        }

        ArpsReadResult result;
        result.status = ArpsReadStatus::Frame;
        result.packet_type = packet.type;
        result.sequence = packet.sequence;
        result.frame.meta = meta;
        result.frame.argb8888 = frame_buffer.data();
        result.frame.argb8888_len = frame_buffer.size();
        result.frame.bitmap_payload_len = packet.bitmap.size();
        result.frame.ext_json = packet.ext;
        result.frame.device_timings = ParseTimings(packet.ext);
        result.frame.packet_read_ms = packet.packet_read_ms;
        result.frame.decode_ms = MsSince(decode_start);
        return result;
    }
};

std::string ArpsStartOptions::ToJson() const {
    std::ostringstream out;
    out << "{"
        << "\"display_id\":" << display_id << ","
        << "\"pixel_format\":\"" << pixel_format << "\","
        << "\"compression\":\"" << compression << "\","
        << "\"max_fps\":" << max_fps << ","
        << "\"max_packet_len\":" << max_packet_len << ","
        << "\"power_on_if_screen_off\":" << (power_on_if_screen_off ? "true" : "false") << ","
        << "\"turn_screen_off\":" << (turn_screen_off ? "true" : "false") << ","
        << "\"keep_screen_on\":" << (keep_screen_on ? "true" : "false") << ","
        << "\"capture_mode\":\"" << capture_mode << "\","
        << "\"exit_power_mode\":\"" << exit_power_mode << "\","
        << "\"stream_mode\":\"" << stream_mode << "\""
        << "}";
    return out.str();
}

ArpsReceiver::ArpsReceiver() : impl_(new Impl()) {}

ArpsReceiver::~ArpsReceiver() {
    Close();
    delete impl_;
}

bool ArpsReceiver::Listen(const std::string& host, std::uint16_t port, std::string* error) {
    Close();
    ArpsSocket socket = sockets::OpenTcpSocket(error);
    if (sockets::IsInvalid(socket)) {
        return false;
    }

    sockets::SetReuseAddr(socket);
    if (!sockets::BindIpv4(socket, host, port, error)) {
        sockets::Close(socket);
        return false;
    }
    if (!sockets::StartListening(socket, 1, error)) {
        sockets::Close(socket);
        return false;
    }
    listen_socket_ = socket;
    return true;
}

bool ArpsReceiver::AcceptOnce(int timeout_ms, std::string* error) {
    if (sockets::IsInvalid(listen_socket_)) {
        if (error) {
            *error = "Listen() must be called before AcceptOnce()";
        }
        return false;
    }
    sockets::WaitStatus wait = sockets::WaitReadable(listen_socket_,
            timeout_ms < 0 ? kNoTimeout : timeout_ms, "wait(accept)", error);
    if (wait == sockets::WaitStatus::Timeout) {
        if (error) {
            *error = "accept timeout";
        }
        return false;
    }
    if (wait == sockets::WaitStatus::Error) {
        return false;
    }
    ArpsSocket socket = sockets::Accept(listen_socket_, error);
    if (sockets::IsInvalid(socket)) {
        return false;
    }
    return AdoptConnectedSocket(socket, error);
}

bool ArpsReceiver::AdoptConnectedSocket(ArpsSocket socket, std::string* error) {
    if (sockets::IsInvalid(socket)) {
        if (error) {
            *error = "invalid connected socket";
        }
        return false;
    }
    if (!sockets::IsInvalid(client_socket_)) {
        sockets::Close(client_socket_);
    }
    client_socket_ = socket;
    sockets::SetTcpNoDelay(client_socket_);
    sockets::SetNoSigpipe(client_socket_);
    return true;
}

bool ArpsReceiver::SendStart(const ArpsStartOptions& options, std::string* error) {
    max_packet_len_ = options.max_packet_len;
    return SendControlPacket(kPacketStart, options.ToJson(), error);
}

bool ArpsReceiver::RequestFrame(std::string* error) {
    return SendControlPacket(kPacketFrameRequest, "{}", error);
}

bool ArpsReceiver::SendControlPacket(std::uint16_t type, const std::string& ext,
        std::string* error) {
    if (sockets::IsInvalid(client_socket_)) {
        if (error) {
            *error = "no connected client";
        }
        return false;
    }
    std::lock_guard<std::mutex> lock(write_mutex_);
    std::vector<std::uint8_t> packet;
    std::uint32_t packet_len = 4 + 0 + 4 + 0 + 4 + static_cast<std::uint32_t>(ext.size());
    packet.insert(packet.end(), kMagic, kMagic + kMagicSize);
    WriteBe16(packet, kProtocolMajor);
    WriteBe16(packet, kProtocolMinor);
    WriteBe16(packet, type);
    WriteBe16(packet, kHeaderLen);
    WriteBe32(packet, 0);
    WriteBe32(packet, next_sequence_++);
    WriteBe32(packet, packet_len);
    WriteBe32(packet, 0);
    WriteBe32(packet, 0);
    WriteBe32(packet, static_cast<std::uint32_t>(ext.size()));
    packet.insert(packet.end(), ext.begin(), ext.end());
    return WriteExact(client_socket_, packet.data(), packet.size(), error) == IoStatus::Ok;
}

ArpsReadResult ArpsReceiver::ReadNext(int timeout_ms) {
    if (sockets::IsInvalid(client_socket_)) {
        ArpsReadResult result;
        result.status = ArpsReadStatus::Closed;
        result.message = "no connected client";
        return result;
    }
    return impl_->ReadPacket(client_socket_, max_packet_len_, timeout_ms);
}

void ArpsReceiver::Close() {
    if (!sockets::IsInvalid(client_socket_)) {
        sockets::Close(client_socket_);
        client_socket_ = kInvalidArpsSocket;
    }
    if (!sockets::IsInvalid(listen_socket_)) {
        sockets::Close(listen_socket_);
        listen_socket_ = kInvalidArpsSocket;
    }
}

}  // namespace arps
