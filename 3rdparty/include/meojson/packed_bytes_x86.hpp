#pragma once
#include "packed_bytes.hpp"

#include <emmintrin.h>
#if defined(__SSE4_1__) || defined(__AVX2__) || defined(_MSC_VER) 
// MSVC enables all SSE4.1 intrinsics by default
#include <smmintrin.h>
#endif

struct packed_bytes_trait_sse {
    static constexpr bool available = true;
    static constexpr auto step = 16;
    using value_type = __m128i;

    __packed_bytes_strong_inline static value_type load_unaligned(const void *ptr) {
        return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
    }

    __packed_bytes_strong_inline static value_type less(value_type x, uint8_t n) {
        auto bcast = _mm_set1_epi8(static_cast<char>(n));
        auto all1 = _mm_set1_epi8(-1);
        auto max_with_n = _mm_max_epu8(x, bcast);
        auto is_greater_or_equal = _mm_cmpeq_epi8(max_with_n, x);
        auto is_less = _mm_andnot_si128(is_greater_or_equal, all1);
        return is_less;
    }

    __packed_bytes_strong_inline static value_type equal(value_type x, uint8_t n) {
        return _mm_cmpeq_epi8(x, _mm_set1_epi8(static_cast<char>(n)));
    }

    __packed_bytes_strong_inline static value_type equal(value_type x, value_type y) {
        return _mm_cmpeq_epi8(x, y);
    }

    __packed_bytes_strong_inline static value_type bitwise_or(value_type a, value_type b) {
        return _mm_or_si128(a, b);
    }

    __packed_bytes_strong_inline static bool is_all_zero(value_type x) {
#if defined(__SSE4_1__) || defined(__AVX2__) || defined(_MSC_VER)
        // SSE4.1 path
        return !!_mm_testz_si128(x, x);
#else
        // SSE2 path
        auto cmp = _mm_cmpeq_epi8(x, _mm_set1_epi8(0));
        auto mask = (uint16_t)_mm_movemask_epi8(cmp);
        return mask == UINT16_C(0xFFFF);
#endif
    }

    __packed_bytes_strong_inline static size_t first_nonzero_byte(value_type x) {
        auto cmp = _mm_cmpeq_epi8(x, _mm_set1_epi8(0));
        auto mask = (uint16_t)_mm_movemask_epi8(cmp);
        return json::__bitops::countr_one((uint32_t)mask);
    }
};

template <>
struct packed_bytes<16> {
    using traits = packed_bytes_trait_sse;
};


#ifdef __AVX2__
#include <immintrin.h>

struct packed_bytes_trait_avx2
{
    static constexpr bool available = true;
    static constexpr auto step = 32;
    using value_type = __m256i;

    __packed_bytes_strong_inline static value_type load_unaligned(const void* ptr)
    {
        return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
    }

    __packed_bytes_strong_inline static value_type less(value_type x, uint8_t n)
    {
        auto bcast = _mm256_set1_epi8(static_cast<char>(n));
        auto all1 = _mm256_set1_epi8(-1);
        auto max_with_n = _mm256_max_epu8(x, bcast);
        auto is_greater_or_equal = _mm256_cmpeq_epi8(max_with_n, x);
        auto is_less = _mm256_andnot_si256(is_greater_or_equal, all1);
        return is_less;
    }

    __packed_bytes_strong_inline static value_type equal(value_type x, uint8_t n)
    {
        return _mm256_cmpeq_epi8(x, _mm256_set1_epi8(static_cast<char>(n)));
    }

    __packed_bytes_strong_inline static value_type equal(value_type x, value_type y)
    {
        return _mm256_cmpeq_epi8(x, y);
    }

    __packed_bytes_strong_inline static value_type bitwise_or(value_type a, value_type b)
    {
        return _mm256_or_si256(a, b);
    }

    __packed_bytes_strong_inline static bool is_all_zero(value_type x)
    {
        return (bool)_mm256_testz_si256(x, x);
    }

    __packed_bytes_strong_inline static size_t first_nonzero_byte(value_type x)
    {
        auto cmp = _mm256_cmpeq_epi8(x, _mm256_set1_epi8(0));
        auto mask = (uint32_t)_mm256_movemask_epi8(cmp);
        // AVX512 alternative: _mm_cmpeq_epi8_mask
        return json::__bitops::countr_one(mask);
    }
};

template <>
struct packed_bytes<32> {
    using traits = packed_bytes_trait_avx2;
};

#endif
