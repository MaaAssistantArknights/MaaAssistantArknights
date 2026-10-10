#include "arps/protocol.h"

namespace arps {

const char* PacketTypeName(std::uint16_t type) {
    switch (type) {
        case kPacketHello:
            return "HELLO";
        case kPacketStart:
            return "START";
        case kPacketReady:
            return "READY";
        case kPacketFrameRequest:
            return "FRAME_REQUEST";
        case kPacketFrame:
            return "FRAME";
        case kPacketError:
            return "ERROR";
        case kPacketStop:
            return "STOP";
        case kPacketPowerControl:
            return "POWER_CONTROL";
        case kPacketPowerState:
            return "POWER_STATE";
        default:
            return "UNKNOWN";
    }
}

const char* CompressionTypeName(std::uint32_t type) {
    switch (type) {
        case kCompressionRaw:
            return "raw";
        case kCompressionLz4Block:
            return "lz4_block";
        case kCompressionDeltaLz4:
            return "delta_lz4";
        case kCompressionExtended:
            return "extended";
        default:
            return "unknown";
    }
}

}  // namespace arps
