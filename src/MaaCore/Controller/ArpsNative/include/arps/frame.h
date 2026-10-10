#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace arps {

struct ArpsFrameMeta {
    std::uint16_t protocol_major = 0;
    std::uint16_t protocol_minor = 0;
    std::uint64_t frame_no = 0;
    std::uint64_t monotonic_time_ns = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t row_bytes = 0;
    std::uint32_t rotation = 0;
    std::uint32_t pixel_format = 0;
    std::uint32_t compression_type = 0;
    std::uint32_t uncompressed_len = 0;
    std::uint32_t compressed_len = 0;
    std::uint32_t display_id = 0;
    std::uint32_t color_space = 0;
    std::uint32_t payload_checksum = 0;
    std::uint32_t flags = 0;
};

struct ArpsDeviceTimings {
    double capture_ms = -1.0;
    double copy_ms = -1.0;
    double compress_ms = -1.0;
    double write_ms = -1.0;
    double previous_write_ms = -1.0;
};

struct ArpsFrame {
    ArpsFrameMeta meta;
    const std::uint8_t* argb8888 = nullptr;
    std::size_t argb8888_len = 0;
    std::size_t bitmap_payload_len = 0;
    std::string ext_json;
    ArpsDeviceTimings device_timings;
    double packet_read_ms = -1.0;
    double decode_ms = -1.0;
};

}  // namespace arps
