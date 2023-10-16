#pragma once

#if __cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#include <bit>
namespace json::__bitops {
    using std::countl_one;
    using std::countr_one;
    using std::countl_zero;
    using std::countr_zero;
    inline constexpr bool is_little_endian() {
        return std::endian::native == std::endian::little;
    }
}
#else
#include <cstdint>
namespace json::__bitops {
#if defined(__GNUC__) || defined(__clang__)
    inline constexpr int countl_zero(uint32_t x) {
        if constexpr (sizeof(uint32_t) == sizeof(unsigned int))
            return x == 0 ? 32 : __builtin_clz(x);
        if constexpr (sizeof(uint32_t) == sizeof(unsigned long))
            return x == 0 ? 32 : __builtin_clzl(x);
        return x == 0 ? 32 : __builtin_clzll(x);
    }
    inline constexpr int countr_zero(uint32_t x) {
        if constexpr (sizeof(uint32_t) == sizeof(unsigned int))
            return x == 0 ? 32 : __builtin_ctz(x);
        if constexpr (sizeof(uint32_t) == sizeof(unsigned long))
            return x == 0 ? 32 : __builtin_ctzl(x);
        return x == 0 ? 32 : __builtin_ctzll(x);
    }
    inline constexpr int countl_zero(uint64_t x) {
        return x == 0 ? 64 : __builtin_clzll(x);
    }
    inline constexpr int countr_zero(uint64_t x) {
        return x == 0 ? 64 : __builtin_ctzll(x);
    }
#elif defined(_MSC_VER)
#ifdef __AVX2__
    // lzcnt intrinsics is not constexpr
    inline int countl_zero(uint32_t x) {
        return __lzcnt(x);
    }
    inline int countr_zero(uint32_t x) {
        return _tzcnt_u32(x);
    }
    inline int countl_zero(uint64_t x) {
        return (int)__lzcnt64(x);
    }
    inline int countr_zero(uint64_t x) {
        return (int)_tzcnt_u64(x);
    }
#else
    inline constexpr int countl_zero(uint32_t x) {
        unsigned long index = 0;
        return _BitScanReverse(&index, x) ? 31 - index : 32;
    }
    inline constexpr int countr_zero(uint32_t x) {
        unsigned long index = 0;
        return _BitScanForward(&index, x) ? index : 32;
    }
    inline constexpr int countl_zero(uint64_t x) {
        unsigned long index = 0;
        return _BitScanReverse64(&index, x) ? 63 - index : 64;
    }
    inline constexpr int countr_zero(uint64_t x) {
        unsigned long index = 0;
        return _BitScanForward64(&index, x) ? index : 64;
    }
#endif // __AVX2__
#else // compiler
#error "bring your own bit counting implementation"
#endif
    inline int countl_one(uint32_t x) { return countl_zero(~x); }
    inline int countr_one(uint32_t x) { return countr_zero(~x); }
    inline int countl_one(uint64_t x) { return countl_zero(~x); }
    inline int countr_one(uint64_t x) { return countr_zero(~x); }

    // no constexpr endian awareness before C++20
    inline bool is_little_endian() {
        union {
            uint32_t u32;
            uint8_t u8;
        } u = { 0x01020304 };
        return u.u8 == 4;
    }
}
#endif // C++20
