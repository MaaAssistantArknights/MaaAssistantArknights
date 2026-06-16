#pragma once

#include <cstddef>
#include <cstdint>

namespace arps {

constexpr char kMagic[] = "ARPSBYVISOTC";
constexpr std::size_t kMagicSize = 12;
constexpr std::uint16_t kProtocolMajor = 1;
constexpr std::uint16_t kProtocolMinor = 0;
constexpr std::uint16_t kHeaderLen = 32;
constexpr std::uint32_t kDefaultMaxPacketLen = 64u * 1024u * 1024u;
constexpr std::size_t kFrameBaseLenV1 = 64;

enum PacketType : std::uint16_t {
    kPacketHello = 1,
    kPacketStart = 2,
    kPacketReady = 3,
    kPacketFrameRequest = 4,
    kPacketFrame = 5,
    kPacketError = 6,
    kPacketStop = 7,
};

enum PixelFormat : std::uint32_t {
    kPixelFormatAndroidArgb8888Raw = 1,
};

enum CompressionType : std::uint32_t {
    kCompressionRaw = 0,
    kCompressionLz4Block = 1,
    kCompressionDeltaLz4 = 2,
    kCompressionExtended = 3,
};

const char* PacketTypeName(std::uint16_t type);
const char* CompressionTypeName(std::uint32_t type);

}  // namespace arps
