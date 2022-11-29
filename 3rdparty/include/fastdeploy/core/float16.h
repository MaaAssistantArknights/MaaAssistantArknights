// Copyright (c) 2022 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>

#include <cmath>
#include <iostream>
#include <limits>

#if !defined(_WIN32)
#define FD_ALIGN(x) __attribute__((aligned(x)))
#else
#define FD_ALIGN(x) __declspec(align(x))
#endif

namespace fastdeploy {

struct FD_ALIGN(2) float16 {
 public:
  uint16_t x;

  // The following defaulted special class member functions
  // are added to make float16 pass the std::is_trivial test
  float16() = default;
  float16(const float16& o) = default;
  float16& operator=(const float16& o) = default;
  float16(float16&& o) = default;
  float16& operator=(float16&& o) = default;
  ~float16() = default;

// Constructors

#ifdef FD_WITH_NATIVE_FP16
  // __fp16 is a native half precision data type for arm cpu,
  // float16_t is an alias for __fp16
  inline explicit float16(const float16_t& h) {
    x = *reinterpret_cast<const uint16_t*>(&h);
  }
#endif

  inline explicit float16(float val) {
#if defined(FD_WITH_NATIVE_FP16)
    float32x4_t tmp = vld1q_dup_f32(&val);
    float16_t res = vget_lane_f16(vcvt_f16_f32(tmp), 0);
    x = *reinterpret_cast<uint16_t*>(&res);

#elif defined(__F16C__)
    x = _cvtss_sh(val, 0);

#else
    // Conversion routine adapted from
    // http://stackoverflow.com/questions/1659440/32-bit-to-16-bit-floating-point-conversion
    Bits v, s;
    v.f = val;
    uint32_t sign = v.si & sigN;
    v.si ^= sign;
    sign >>= shiftSign;  // logical shift
    s.si = mulN;
    s.si = s.f * v.f;  // correct subnormals
    v.si ^= (s.si ^ v.si) & -(minN > v.si);
    v.si ^= (infN ^ v.si) & -((infN > v.si) & (v.si > maxN));
    v.si ^= (nanN ^ v.si) & -((nanN > v.si) & (v.si > infN));
    v.ui >>= shift;  // logical shift
    v.si ^= ((v.si - maxD) ^ v.si) & -(v.si > maxC);
    v.si ^= ((v.si - minD) ^ v.si) & -(v.si > subC);
    x = v.ui | sign;

#endif
  }

  inline explicit float16(bool b) : x(b ? 0x3c00 : 0) {}

  template <class T>
  inline explicit float16(const T& val)
      : x(float16(static_cast<float>(val)).x) {}

// Assignment operators

#ifdef FD_WITH_NATIVE_FP16
  inline float16& operator=(const float16_t& rhs) {
    x = *reinterpret_cast<const uint16_t*>(&rhs);
    return *this;
  }
#endif

  inline float16& operator=(bool b) {
    x = b ? 0x3c00 : 0;
    return *this;
  }

  inline float16& operator=(int8_t val) {
    x = float16(val).x;
    return *this;
  }

  inline float16& operator=(uint8_t val) {
    x = float16(val).x;
    return *this;
  }

  inline float16& operator=(int16_t val) {
    x = float16(val).x;
    return *this;
  }

  inline float16& operator=(uint16_t val) {
    x = float16(val).x;
    return *this;
  }

  inline float16& operator=(int32_t val) {
    x = float16(val).x;
    return *this;
  }

  inline float16& operator=(uint32_t val) {
    x = float16(val).x;
    return *this;
  }

  inline float16& operator=(int64_t val) {
    x = float16(val).x;
    return *this;
  }

  inline float16& operator=(uint64_t val) {
    x = float16(val).x;
    return *this;
  }

  inline float16& operator=(float val) {
    x = float16(val).x;
    return *this;
  }

  inline float16& operator=(double val) {
    x = float16(val).x;
    return *this;
  }

// Conversion opertors
#ifdef FD_WITH_NATIVE_FP16
  HOSTDEVICE inline explicit operator float16_t() const {
    return *reinterpret_cast<const float16_t*>(this);
  }
#endif

  inline operator float() const {
#if defined(FD_WITH_NATIVE_FP16)
    float16x4_t res = vld1_dup_f16(reinterpret_cast<const float16_t*>(this));
    return vgetq_lane_f32(vcvt_f32_f16(res), 0);

#elif defined(__F16C__)
    return _cvtsh_ss(this->x);

#else
    // Conversion routine adapted from
    // http://stackoverflow.com/questions/1659440/32-bit-to-16-bit-floating-point-conversion
    Bits v;
    v.ui = this->x;
    int32_t sign = v.si & sigC;
    v.si ^= sign;
    sign <<= shiftSign;
    v.si ^= ((v.si + minD) ^ v.si) & -(v.si > subC);
    v.si ^= ((v.si + maxD) ^ v.si) & -(v.si > maxC);
    Bits s;
    s.si = mulC;
    s.f *= v.si;
    int32_t mask = -(norC > v.si);
    v.si <<= shift;
    v.si ^= (s.si ^ v.si) & mask;
    v.si |= sign;
    return v.f;

#endif
  }

  inline explicit operator bool() const { return (x & 0x7fff) != 0; }

  inline explicit operator int8_t() const {
    return static_cast<int8_t>(static_cast<float>(*this));
  }

  inline explicit operator uint8_t() const {
    return static_cast<uint8_t>(static_cast<float>(*this));
  }

  inline explicit operator int16_t() const {
    return static_cast<int16_t>(static_cast<float>(*this));
  }

  inline explicit operator uint16_t() const {
    return static_cast<uint16_t>(static_cast<float>(*this));
  }

  inline explicit operator int32_t() const {
    return static_cast<int32_t>(static_cast<float>(*this));
  }

  inline explicit operator uint32_t() const {
    return static_cast<uint32_t>(static_cast<float>(*this));
  }

  inline explicit operator int64_t() const {
    return static_cast<int64_t>(static_cast<float>(*this));
  }

  inline explicit operator uint64_t() const {
    return static_cast<uint64_t>(static_cast<float>(*this));
  }

  inline operator double() const {
    return static_cast<double>(static_cast<float>(*this));
  }

  inline bool operator>(const float& other) const {
    return this->operator float() > other;
  }

  inline bool operator>(const double& other) const {
    return this->operator double() > other;
  }

  inline bool operator<(const float& other) const {
    return this->operator float() > other;
  }

  inline bool operator<(const double& other) const {
    return this->operator double() > other;
  }

  template <typename T,
            typename std::enable_if<!std::is_same<T, float16>::value,
                                    bool>::type = true>
  inline float16& operator+=(const T& other) {
    *this = float16(static_cast<T>(*this) + other);
    return *this;
  }

 private:
  union Bits {
    float f;
    int32_t si;
    uint32_t ui;
  };

  static const int shift = 13;
  static const int shiftSign = 16;

  static const int32_t infN = 0x7F800000;
  static const int32_t maxN = 0x477FE000;  // max flt16 as flt32
  static const int32_t minN = 0x38800000;  // min flt16 normal as flt32
  static const int32_t sigN = 0x80000000;  // sign bit

  static constexpr int32_t infC = infN >> shift;
  static constexpr int32_t nanN = (infC + 1)
                                  << shift;  // minimum flt16 nan as float32
  static constexpr int32_t maxC = maxN >> shift;
  static constexpr int32_t minC = minN >> shift;
  static constexpr int32_t sigC = sigN >> shiftSign;

  static const int32_t mulN = 0x52000000;  // (1 << 23) / minN
  static const int32_t mulC = 0x33800000;  // minN / (1 << (23 - shift))
  static const int32_t subC = 0x003FF;     // max flt32 subnormal downshifted
  static const int32_t norC = 0x00400;     // min flt32 normal downshifted

  static constexpr int32_t maxD = infC - maxC - 1;
  static constexpr int32_t minD = minC - subC - 1;
};

// Arithmetic operators for float16 on ARMv8.2-A CPU
#if defined(FD_WITH_NATIVE_FP16)
inline float16 operator+(const float16& a, const float16& b) {
  float16 res;
  asm volatile(
      "ld1 {v0.h}[0], [%[a_ptr]]\n"
      "ld1 {v1.h}[0], [%[b_ptr]]\n"
      "fadd h0, h0, h1\n"
      "st1 {v0.h}[0], [%[res_ptr]]\n"
      :  // outputs
      :  // inputs
      [a_ptr] "r"(&(a.x)), [b_ptr] "r"(&(b.x)),
      [res_ptr] "r"(&(res.x))
      :  // clobbers
      "memory", "v0", "v1");
  return res;
}

inline float16 operator-(const float16& a, const float16& b) {
  float16 res;
  asm volatile(
      "ld1 {v0.h}[0], [%[a_ptr]]\n"
      "ld1 {v1.h}[0], [%[b_ptr]]\n"
      "fsub h0, h0, h1\n"
      "st1 {v0.h}[0], [%[res_ptr]]\n"
      :  // outputs
      :  // inputs
      [a_ptr] "r"(&(a.x)), [b_ptr] "r"(&(b.x)),
      [res_ptr] "r"(&(res.x))
      :  // clobbers
      "memory", "v0", "v1");
  return res;
}

inline float16 operator*(const float16& a, const float16& b) {
  float16 res;
  asm volatile(
      "ld1 {v0.h}[0], [%[a_ptr]]\n"
      "ld1 {v1.h}[0], [%[b_ptr]]\n"
      "fmul h0, h0, h1\n"
      "st1 {v0.h}[0], [%[res_ptr]]\n"
      :  // outputs
      :  // inputs
      [a_ptr] "r"(&(a.x)), [b_ptr] "r"(&(b.x)),
      [res_ptr] "r"(&(res.x))
      :  // clobbers
      "memory", "v0", "v1");
  return res;
}

inline float16 operator/(const float16& a, const float16& b) {
  float16 res;
  asm volatile(
      "ld1 {v0.h}[0], [%[a_ptr]]\n"
      "ld1 {v1.h}[0], [%[b_ptr]]\n"
      "fdiv h0, h0, h1\n"
      "st1 {v0.h}[0], [%[res_ptr]]\n"
      :  // outputs
      :  // inputs
      [a_ptr] "r"(&(a.x)), [b_ptr] "r"(&(b.x)),
      [res_ptr] "r"(&(res.x))
      :  // clobbers
      "memory", "v0", "v1");
  return res;
}

inline float16 operator-(const float16& a) {
  float16 res;
  asm volatile(
      "ld1 {v0.h}[0], [%[a_ptr]]\n"
      "fneg h0, h0\n"
      "st1 {v0.h}[0], [%[res_ptr]]\n"
      :  // outputs
      :  // inputs
      [a_ptr] "r"(&(a.x)),
      [res_ptr] "r"(&(res.x))
      :  // clobbers
      "memory", "v0");
  return res;
}

inline float16& operator+=(float16& a, const float16& b) {  // NOLINT
  a = a + b;
  return a;
}

inline float16& operator-=(float16& a, const float16& b) {  // NOLINT
  a = a - b;
  return a;
}

inline float16& operator*=(float16& a, const float16& b) {  // NOLINT
  a = a * b;
  return a;
}

inline float16& operator/=(float16& a, const float16& b) {  // NOLINT
  a = a / b;
  return a;
}

inline bool operator==(const float16& a, const float16& b) {
  uint16_t res;
  asm volatile(
      "ld1 {v0.h}[0], [%[a_ptr]]\n"
      "ld1 {v1.h}[0], [%[b_ptr]]\n"
      "fcmeq h0, h0, h1\n"
      "st1 {v0.h}[0], [%[res_ptr]]\n"
      :  // outputs
      :  // inputs
      [a_ptr] "r"(&(a.x)), [b_ptr] "r"(&(b.x)),
      [res_ptr] "r"(&res)
      :  // clobbers
      "memory", "v0", "v1");
  return (res & 0xffff) != 0;
}

inline bool operator!=(const float16& a, const float16& b) { return !(a == b); }

inline bool operator<(const float16& a, const float16& b) {
  uint16_t res;
  asm volatile(
      "ld1 {v1.h}[0], [%[a_ptr]]\n"
      "ld1 {v0.h}[0], [%[b_ptr]]\n"
      "fcmgt h0, h0, h1\n"
      "st1 {v0.h}[0], [%[res_ptr]]\n"
      :  // outputs
      :  // inputs
      [a_ptr] "r"(&(a.x)), [b_ptr] "r"(&(b.x)),
      [res_ptr] "r"(&res)
      :  // clobbers
      "memory", "v0", "v1");
  return (res & 0xffff) != 0;
}

inline bool operator<=(const float16& a, const float16& b) {
  uint16_t res;
  asm volatile(
      "ld1 {v1.h}[0], [%[a_ptr]]\n"
      "ld1 {v0.h}[0], [%[b_ptr]]\n"
      "fcmge h0, h0, h1\n"
      "st1 {v0.h}[0], [%[res_ptr]]\n"
      :  // outputs
      :  // inputs
      [a_ptr] "r"(&(a.x)), [b_ptr] "r"(&(b.x)),
      [res_ptr] "r"(&res)
      :  // clobbers
      "memory", "v0", "v1");
  return (res & 0xffff) != 0;
}

inline bool operator>(const float16& a, const float16& b) {
  uint16_t res;
  asm volatile(
      "ld1 {v0.h}[0], [%[a_ptr]]\n"
      "ld1 {v1.h}[0], [%[b_ptr]]\n"
      "fcmgt h0, h0, h1\n"
      "st1 {v0.h}[0], [%[res_ptr]]\n"
      :  // outputs
      :  // inputs
      [a_ptr] "r"(&(a.x)), [b_ptr] "r"(&(b.x)),
      [res_ptr] "r"(&res)
      :  // clobbers
      "memory", "v0", "v1");
  return (res & 0xffff) != 0;
}

inline bool operator>=(const float16& a, const float16& b) {
  uint16_t res;
  asm volatile(
      "ld1 {v0.h}[0], [%[a_ptr]]\n"
      "ld1 {v1.h}[0], [%[b_ptr]]\n"
      "fcmge h0, h0, h1\n"
      "st1 {v0.h}[0], [%[res_ptr]]\n"
      :  // outputs
      :  // inputs
      [a_ptr] "r"(&(a.x)), [b_ptr] "r"(&(b.x)),
      [res_ptr] "r"(&res)
      :  // clobbers
      "memory", "v0", "v1");
  return (res & 0xffff) != 0;
#else
inline float16 operator+(const float16& a, const float16& b) {
  return float16(static_cast<float>(a) + static_cast<float>(b));
}

inline float16 operator-(const float16& a, const float16& b) {
  return float16(static_cast<float>(a) - static_cast<float>(b));
}

inline float16 operator*(const float16& a, const float16& b) {
  return float16(static_cast<float>(a) * static_cast<float>(b));
}

inline float16 operator/(const float16& a, const float16& b) {
  return float16(static_cast<float>(a) / static_cast<float>(b));
}

inline float16 operator-(const float16& a) {
  float16 res;
  res.x = a.x ^ 0x8000;
  return res;
}

inline float16& operator+=(float16& a, const float16& b) {  // NOLINT
  a = float16(static_cast<float>(a) + static_cast<float>(b));
  return a;
}

inline float16& operator-=(float16& a, const float16& b) {  // NOLINT
  a = float16(static_cast<float>(a) - static_cast<float>(b));
  return a;
}

inline float16& operator*=(float16& a, const float16& b) {  // NOLINT
  a = float16(static_cast<float>(a) * static_cast<float>(b));
  return a;
}

inline float16& operator/=(float16& a, const float16& b) {  // NOLINT
  a = float16(static_cast<float>(a) / static_cast<float>(b));
  return a;
}

inline bool operator==(const float16& a, const float16& b) {
  return static_cast<float>(a) == static_cast<float>(b);
}

inline bool operator!=(const float16& a, const float16& b) {
  return static_cast<float>(a) != static_cast<float>(b);
}

inline bool operator<(const float16& a, const float16& b) {
  return static_cast<float>(a) < static_cast<float>(b);
}

inline bool operator<=(const float16& a, const float16& b) {
  return static_cast<float>(a) <= static_cast<float>(b);
}

inline bool operator>(const float16& a, const float16& b) {
  return static_cast<float>(a) > static_cast<float>(b);
}

inline bool operator>=(const float16& a, const float16& b) {
  return static_cast<float>(a) >= static_cast<float>(b);
}
#endif

  template <typename T,
            typename std::enable_if<std::is_integral<T>::value ||
                                        std::is_same<T, float>::value,
                                    bool>::type = true>
  inline T& operator+=(T& a, const float16& b) {  // NOLINT
    auto c = static_cast<float>(a) + static_cast<float>(b);
    a = static_cast<T>(c);
    return a;
  }

  inline double& operator+=(double& a, const float16& b) {  // NOLINT
    a = a + static_cast<double>(b);
    return a;
  }

  inline float16 raw_uint16_to_float16(uint16_t a) {
    float16 res;
    res.x = a;
    return res;
  }

  inline bool(isnan)(const float16& a) { return (a.x & 0x7fff) > 0x7c00; }

  inline bool(isinf)(const float16& a) { return (a.x & 0x7fff) == 0x7c00; }

  inline bool(isfinite)(const float16& a) {
    return !((isnan)(a)) && !((isinf)(a));
  }

  inline float16(abs)(const float16& a) {
    return float16(std::abs(static_cast<float>(a)));
  }

  inline std::ostream& operator<<(std::ostream& os, const float16& a) {
    os << static_cast<float>(a);
    return os;
  }
}  // namespace fastdeploy

namespace std {

// Override the std::is_pod::value for float16
// The reason is that different compilers implemented std::is_pod based on
// different C++ standards. float16 class is a plain old data in C++11 given
// that it is both trivial and standard_layout.
// However, std::is_pod in nvcc 8.0 host c++ compiler follows C++0x and is
// more restricted in that you cannot provide any customized
// constructor in float16. Hence, we override is_pod here following C++11
// so that .cu files can be successfully compiled by nvcc.
template <>
struct is_pod<fastdeploy::float16> {
  static const bool value = is_trivial<fastdeploy::float16>::value &&
                            is_standard_layout<fastdeploy::float16>::value;
};

template <>
struct is_floating_point<fastdeploy::float16>
    : std::integral_constant<
          bool, std::is_same<fastdeploy::float16,
                             typename std::remove_cv<
                                 fastdeploy::float16>::type>::value> {};
template <>
struct is_signed<fastdeploy::float16> {
  static const bool value = true;
};

template <>
struct is_unsigned<fastdeploy::float16> {
  static const bool value = false;
};

inline bool isnan(const fastdeploy::float16& a) { return fastdeploy::isnan(a); }

inline bool isinf(const fastdeploy::float16& a) { return fastdeploy::isinf(a); }

template <>
struct numeric_limits<fastdeploy::float16> {
  static const bool is_specialized = true;
  static const bool is_signed = true;
  static const bool is_integer = false;
  static const bool is_exact = false;
  static const bool has_infinity = true;
  static const bool has_quiet_NaN = true;
  static const bool has_signaling_NaN = true;
  static const float_denorm_style has_denorm = denorm_present;
  static const bool has_denorm_loss = false;
  static const std::float_round_style round_style = std::round_to_nearest;
  static const bool is_iec559 = false;
  static const bool is_bounded = false;
  static const bool is_modulo = false;
  static const int digits = 11;
  static const int digits10 = 3;
  static const int max_digits10 = 5;
  static const int radix = 2;
  static const int min_exponent = -13;
  static const int min_exponent10 = -4;
  static const int max_exponent = 16;
  static const int max_exponent10 = 4;
  static const bool traps = true;
  static const bool tinyness_before = false;

  static fastdeploy::float16(min)() {
    return fastdeploy::raw_uint16_to_float16(0x400);
  }
  static fastdeploy::float16 lowest() {
    return fastdeploy::raw_uint16_to_float16(0xfbff);
  }
  static fastdeploy::float16(max)() {
    return fastdeploy::raw_uint16_to_float16(0x7bff);
  }
  static fastdeploy::float16 epsilon() {
    return fastdeploy::raw_uint16_to_float16(0x0800);
  }
  static fastdeploy::float16 round_error() { return fastdeploy::float16(0.5); }
  static fastdeploy::float16 infinity() {
    return fastdeploy::raw_uint16_to_float16(0x7c00);
  }
  static fastdeploy::float16 quiet_NaN() {
    return fastdeploy::raw_uint16_to_float16(0x7e00);
  }
  static fastdeploy::float16 signaling_NaN() {
    return fastdeploy::raw_uint16_to_float16(0x7e00);
  }
  static fastdeploy::float16 denorm_min() {
    return fastdeploy::raw_uint16_to_float16(0x1);
  }
};

inline fastdeploy::float16 abs(const fastdeploy::float16& a) {
  return fastdeploy::abs(a);
}

}  // namespace std
