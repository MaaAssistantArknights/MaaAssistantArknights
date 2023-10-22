#pragma once

// current NEON implementation doesn't outperform 64-bit scalar implementation
#ifdef MEOJSON_ENABLE_NEON
#include "packed_bytes.hpp"
#include <arm_neon.h>

#if defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
#define __packed_bytes_trait_arm64
#endif

struct packed_bytes_trait_neon {
    static constexpr bool available = true;
    static constexpr auto step = 16;
    using value_type = uint8x16_t;

    __packed_bytes_strong_inline static value_type load_unaligned(const void *ptr) {
        return vld1q_u8((uint8_t*)ptr);
    }

    __packed_bytes_strong_inline static value_type less(value_type x, uint8_t n) {
        auto bcast = vdupq_n_u8(n);
        auto is_less = vcltq_u8(x, bcast);
        return is_less;
    }

    __packed_bytes_strong_inline static value_type equal(value_type x, uint8_t n) {
        return vceqq_u8(x, vdupq_n_u8(n));
    }

    __packed_bytes_strong_inline static value_type equal(value_type x, value_type y) {
        return vceqq_u8(x, y);
    }

    __packed_bytes_strong_inline static value_type bitwise_or(value_type a, value_type b) {
        return vorrq_u8(a, b);
    }

    __packed_bytes_strong_inline static bool is_all_zero(value_type x) {
#ifdef __packed_bytes_trait_arm64
        return vmaxvq_u8(x) == 0;
#else
        auto fold64 = vorr_u64(vget_high_u64(vreinterpretq_u8_u64(x), 0), vget_low_u64(vreinterpretq_u8_u64(x), 1));
        auto fold32 = vget_lane_u32(vreinterpret_u64_u32(fold64), 0) | vget_lane_u32(vreinterpret_u64_u32(fold64), 1);
        return fold32 == 0;
#endif
    }

    __packed_bytes_strong_inline static size_t first_nonzero_byte(value_type x) {
        // https://community.arm.com/arm-community-blogs/b/infrastructure-solutions-blog/posts/porting-x86-vector-bitmask-optimizations-to-arm-neon
        auto cmp = equal(x, 0);
        auto res = vshrn_n_u16(cmp, 4);
        auto mask64 = vget_lane_u64(vreinterpret_u64_u8(res), 0);
        return json::__bitops::countr_one(mask64) >> 2;
    }
};

template <>
struct packed_bytes<16> {
    using traits = packed_bytes_trait_neon;
};

#endif
