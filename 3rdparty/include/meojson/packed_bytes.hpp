#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "bitops.hpp"

#if defined(__GNUC__) || defined(__clang__)
#define __packed_bytes_strong_inline __attribute__((always_inline)) 
#elif defined(_MSC_VER)
#define __packed_bytes_strong_inline __forceinline
#else
#define __packed_bytes_strong_inline inline
#endif

struct packed_bytes_trait_none {
    static constexpr bool available = false;
};

template <size_t N>
struct packed_bytes {
    using traits = packed_bytes_trait_none;
};

#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP)
#include "packed_bytes_x86.hpp"
#elif defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64)
#include "packed_bytes_arm.hpp"
#endif

struct packed_bytes_trait_uint64 {
    static constexpr bool available = sizeof(void*) >= 8;
    static constexpr auto step = 8;
    using value_type = std::enable_if_t<sizeof(void*) >= 8, uint64_t>;

    __packed_bytes_strong_inline static value_type load_unaligned(const void *ptr) {
        value_type result;
        memcpy((void*)&result, ptr, 8);
        return result;
    }

    __packed_bytes_strong_inline static value_type less(value_type x, uint8_t n) {
        return (((x) - UINT64_C(0x0101010101010101) * (n)) & ~(x) & UINT64_C(0x8080808080808080));
    }

    __packed_bytes_strong_inline static value_type is_zero_memberwise(value_type v) {
        return (((v) - UINT64_C(0x0101010101010101)) & ~(v) & UINT64_C(0x8080808080808080));
    }

    __packed_bytes_strong_inline static bool is_all_zero(value_type v)
    {
        return v == UINT64_C(0);
    }

    __packed_bytes_strong_inline static value_type equal(value_type x, uint8_t n) {
        return is_zero_memberwise((x) ^ (UINT64_C(0x0101010101010101) * (n)));
    }

    __packed_bytes_strong_inline static value_type bitwise_or(value_type a, value_type b) {
        return a | b;
    }

    __packed_bytes_strong_inline static size_t first_nonzero_byte(value_type x) {
        if (json::__bitops::is_little_endian())
            return json::__bitops::countr_zero(x) / 8;
        else
            return json::__bitops::countl_zero(x) / 8;
    }
};

struct packed_bytes_trait_uint32 {
    static constexpr bool available = true;
    static constexpr auto step = 4;
    using value_type = uint32_t;

    __packed_bytes_strong_inline static value_type load_unaligned(const void *ptr) {
        value_type result;
        memcpy((void*)&result, ptr, 4);
        return result;
    }

    __packed_bytes_strong_inline static value_type less(value_type x, uint8_t n) {
        return (((x) - ~UINT32_C(0) / 255 * (n)) & ~(x) & ~UINT32_C(0) / 255 * 128);
    }

    __packed_bytes_strong_inline static value_type is_zero_memberwise(value_type v) {
        return (((v) - UINT32_C(0x01010101)) & ~(v) & UINT32_C(0x80808080));;
    }

    __packed_bytes_strong_inline static bool is_all_zero(value_type v) {
        return v == UINT32_C(0);
    }

    __packed_bytes_strong_inline static value_type equal(value_type x, uint8_t n) {
        return is_zero_memberwise((x) ^ (~UINT32_C(0) / 255 * (n)));
    }

    __packed_bytes_strong_inline static value_type bitwise_or(value_type a, value_type b) {
        return a | b;
    }

    __packed_bytes_strong_inline static size_t first_nonzero_byte(value_type x) {
        if (json::__bitops::is_little_endian())
            return json::__bitops::countr_zero(x) / 8;
        else
            return json::__bitops::countl_zero(x) / 8;
    }
};
template <>
struct packed_bytes<8> {
    using traits = std::enable_if_t<packed_bytes_trait_uint64::available, packed_bytes_trait_uint64>;
};

template <>
struct packed_bytes<4> {
    using traits = packed_bytes_trait_uint32;
};

template <size_t N>
using packed_bytes_trait = typename packed_bytes<N>::traits;

using packed_bytes_trait_max = std::conditional_t<packed_bytes_trait<32>::available, packed_bytes_trait<32>,
  std::conditional_t<packed_bytes_trait<16>::available, packed_bytes_trait<16>,
    std::conditional_t<packed_bytes_trait<8>::available, packed_bytes_trait<8>,
      packed_bytes_trait<4>
  >>>;

