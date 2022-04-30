// //////////////////////////////////////////////////////////
// md5.h
// Copyright (c) 2014 Stephan Brumme. All rights reserved.
// see http://create.stephan-brumme.com/disclaimer.html
//

#pragma once

//#include "hash.h"
#include <string>

// define fixed size integer types
#ifdef _MSC_VER
// Windows
typedef unsigned __int8 uint8_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
// GCC
#include <stdint.h>
#endif

/// compute MD5 hash
/** Usage:
    MD5 md5;
    std::string myHash  = md5("Hello World");     // std::string
    std::string myHash2 = md5("How are you", 11); // arbitrary data, 11 bytes
    // or in a streaming fashion:
    MD5 md5;
    while (more data available)
      md5.add(pointer to fresh data, number of new bytes);
    std::string myHash3 = md5.getHash();
  */
class MD5 //: public Hash
{
public:
    /// split into 64 byte blocks (=> 512 bits), hash is 16 bytes long
    enum { BlockSize = 512 / 8,
        HashBytes = 16 };

    /// same as reset()
    MD5();

    /// compute MD5 of a memory block
    std::string operator()(const void* data, size_t numBytes);
    /// compute MD5 of a string, excluding final zero
    std::string operator()(const std::string& text);

    /// add arbitrary number of bytes
    void add(const void* data, size_t numBytes);

    /// return latest hash as 32 hex characters
    std::string getHash();
    /// return latest hash as bytes
    void getHash(unsigned char buffer[HashBytes]);

    /// restart
    void reset();

private:
    /// process 64 bytes
    void processBlock(const void* data);
    /// process everything left in the internal buffer
    void processBuffer();

    /// size of processed data in bytes
    uint64_t m_numBytes;
    /// valid bytes in m_buffer
    size_t m_bufferSize;
    /// bytes not processed yet
    uint8_t m_buffer[BlockSize];

    enum { HashValues = HashBytes / 4 };
    /// hash, stored as integers
    uint32_t m_hash[HashValues];
};

/// same as reset()
MD5::MD5()
{
    reset();
}

/// restart
void MD5::reset()
{
    m_numBytes = 0;
    m_bufferSize = 0;

    // according to RFC 1321
    m_hash[0] = 0x67452301;
    m_hash[1] = 0xefcdab89;
    m_hash[2] = 0x98badcfe;
    m_hash[3] = 0x10325476;
}

namespace {
// mix functions for processBlock()
inline uint32_t f1(uint32_t b, uint32_t c, uint32_t d)
{
    return d ^ (b & (c ^ d)); // original: f = (b & c) | ((~b) & d);
}

inline uint32_t f2(uint32_t b, uint32_t c, uint32_t d)
{
    return c ^ (d & (b ^ c)); // original: f = (b & d) | (c & (~d));
}

inline uint32_t f3(uint32_t b, uint32_t c, uint32_t d)
{
    return b ^ c ^ d;
}

inline uint32_t f4(uint32_t b, uint32_t c, uint32_t d)
{
    return c ^ (b | ~d);
}

inline uint32_t rotate(uint32_t a, uint32_t c)
{
    return (a << c) | (a >> (32 - c));
}

#if defined(__BYTE_ORDER) && (__BYTE_ORDER != 0) && (__BYTE_ORDER == __BIG_ENDIAN)
inline uint32_t swap(uint32_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(x);
#endif
#ifdef MSC_VER
    return _byteswap_ulong(x);
#endif

    return (x >> 24) | ((x >> 8) & 0x0000FF00) | ((x << 8) & 0x00FF0000) | (x << 24);
}
#endif
}

/// process 64 bytes
void MD5::processBlock(const void* data)
{
    // get last hash
    uint32_t a = m_hash[0];
    uint32_t b = m_hash[1];
    uint32_t c = m_hash[2];
    uint32_t d = m_hash[3];

    // data represented as 16x 32-bit words
    const uint32_t* words = (uint32_t*)data;

    // computations are little endian, swap data if necessary
#if defined(__BYTE_ORDER) && (__BYTE_ORDER != 0) && (__BYTE_ORDER == __BIG_ENDIAN)
#define LITTLEENDIAN(x) swap(x)
#else
#define LITTLEENDIAN(x) (x)
#endif

    // first round
    uint32_t word0 = LITTLEENDIAN(words[0]);
    a = rotate(a + f1(b, c, d) + word0 + 0xd76aa478, 7) + b;
    uint32_t word1 = LITTLEENDIAN(words[1]);
    d = rotate(d + f1(a, b, c) + word1 + 0xe8c7b756, 12) + a;
    uint32_t word2 = LITTLEENDIAN(words[2]);
    c = rotate(c + f1(d, a, b) + word2 + 0x242070db, 17) + d;
    uint32_t word3 = LITTLEENDIAN(words[3]);
    b = rotate(b + f1(c, d, a) + word3 + 0xc1bdceee, 22) + c;

    uint32_t word4 = LITTLEENDIAN(words[4]);
    a = rotate(a + f1(b, c, d) + word4 + 0xf57c0faf, 7) + b;
    uint32_t word5 = LITTLEENDIAN(words[5]);
    d = rotate(d + f1(a, b, c) + word5 + 0x4787c62a, 12) + a;
    uint32_t word6 = LITTLEENDIAN(words[6]);
    c = rotate(c + f1(d, a, b) + word6 + 0xa8304613, 17) + d;
    uint32_t word7 = LITTLEENDIAN(words[7]);
    b = rotate(b + f1(c, d, a) + word7 + 0xfd469501, 22) + c;

    uint32_t word8 = LITTLEENDIAN(words[8]);
    a = rotate(a + f1(b, c, d) + word8 + 0x698098d8, 7) + b;
    uint32_t word9 = LITTLEENDIAN(words[9]);
    d = rotate(d + f1(a, b, c) + word9 + 0x8b44f7af, 12) + a;
    uint32_t word10 = LITTLEENDIAN(words[10]);
    c = rotate(c + f1(d, a, b) + word10 + 0xffff5bb1, 17) + d;
    uint32_t word11 = LITTLEENDIAN(words[11]);
    b = rotate(b + f1(c, d, a) + word11 + 0x895cd7be, 22) + c;

    uint32_t word12 = LITTLEENDIAN(words[12]);
    a = rotate(a + f1(b, c, d) + word12 + 0x6b901122, 7) + b;
    uint32_t word13 = LITTLEENDIAN(words[13]);
    d = rotate(d + f1(a, b, c) + word13 + 0xfd987193, 12) + a;
    uint32_t word14 = LITTLEENDIAN(words[14]);
    c = rotate(c + f1(d, a, b) + word14 + 0xa679438e, 17) + d;
    uint32_t word15 = LITTLEENDIAN(words[15]);
    b = rotate(b + f1(c, d, a) + word15 + 0x49b40821, 22) + c;

    // second round
    a = rotate(a + f2(b, c, d) + word1 + 0xf61e2562, 5) + b;
    d = rotate(d + f2(a, b, c) + word6 + 0xc040b340, 9) + a;
    c = rotate(c + f2(d, a, b) + word11 + 0x265e5a51, 14) + d;
    b = rotate(b + f2(c, d, a) + word0 + 0xe9b6c7aa, 20) + c;

    a = rotate(a + f2(b, c, d) + word5 + 0xd62f105d, 5) + b;
    d = rotate(d + f2(a, b, c) + word10 + 0x02441453, 9) + a;
    c = rotate(c + f2(d, a, b) + word15 + 0xd8a1e681, 14) + d;
    b = rotate(b + f2(c, d, a) + word4 + 0xe7d3fbc8, 20) + c;

    a = rotate(a + f2(b, c, d) + word9 + 0x21e1cde6, 5) + b;
    d = rotate(d + f2(a, b, c) + word14 + 0xc33707d6, 9) + a;
    c = rotate(c + f2(d, a, b) + word3 + 0xf4d50d87, 14) + d;
    b = rotate(b + f2(c, d, a) + word8 + 0x455a14ed, 20) + c;

    a = rotate(a + f2(b, c, d) + word13 + 0xa9e3e905, 5) + b;
    d = rotate(d + f2(a, b, c) + word2 + 0xfcefa3f8, 9) + a;
    c = rotate(c + f2(d, a, b) + word7 + 0x676f02d9, 14) + d;
    b = rotate(b + f2(c, d, a) + word12 + 0x8d2a4c8a, 20) + c;

    // third round
    a = rotate(a + f3(b, c, d) + word5 + 0xfffa3942, 4) + b;
    d = rotate(d + f3(a, b, c) + word8 + 0x8771f681, 11) + a;
    c = rotate(c + f3(d, a, b) + word11 + 0x6d9d6122, 16) + d;
    b = rotate(b + f3(c, d, a) + word14 + 0xfde5380c, 23) + c;

    a = rotate(a + f3(b, c, d) + word1 + 0xa4beea44, 4) + b;
    d = rotate(d + f3(a, b, c) + word4 + 0x4bdecfa9, 11) + a;
    c = rotate(c + f3(d, a, b) + word7 + 0xf6bb4b60, 16) + d;
    b = rotate(b + f3(c, d, a) + word10 + 0xbebfbc70, 23) + c;

    a = rotate(a + f3(b, c, d) + word13 + 0x289b7ec6, 4) + b;
    d = rotate(d + f3(a, b, c) + word0 + 0xeaa127fa, 11) + a;
    c = rotate(c + f3(d, a, b) + word3 + 0xd4ef3085, 16) + d;
    b = rotate(b + f3(c, d, a) + word6 + 0x04881d05, 23) + c;

    a = rotate(a + f3(b, c, d) + word9 + 0xd9d4d039, 4) + b;
    d = rotate(d + f3(a, b, c) + word12 + 0xe6db99e5, 11) + a;
    c = rotate(c + f3(d, a, b) + word15 + 0x1fa27cf8, 16) + d;
    b = rotate(b + f3(c, d, a) + word2 + 0xc4ac5665, 23) + c;

    // fourth round
    a = rotate(a + f4(b, c, d) + word0 + 0xf4292244, 6) + b;
    d = rotate(d + f4(a, b, c) + word7 + 0x432aff97, 10) + a;
    c = rotate(c + f4(d, a, b) + word14 + 0xab9423a7, 15) + d;
    b = rotate(b + f4(c, d, a) + word5 + 0xfc93a039, 21) + c;

    a = rotate(a + f4(b, c, d) + word12 + 0x655b59c3, 6) + b;
    d = rotate(d + f4(a, b, c) + word3 + 0x8f0ccc92, 10) + a;
    c = rotate(c + f4(d, a, b) + word10 + 0xffeff47d, 15) + d;
    b = rotate(b + f4(c, d, a) + word1 + 0x85845dd1, 21) + c;

    a = rotate(a + f4(b, c, d) + word8 + 0x6fa87e4f, 6) + b;
    d = rotate(d + f4(a, b, c) + word15 + 0xfe2ce6e0, 10) + a;
    c = rotate(c + f4(d, a, b) + word6 + 0xa3014314, 15) + d;
    b = rotate(b + f4(c, d, a) + word13 + 0x4e0811a1, 21) + c;

    a = rotate(a + f4(b, c, d) + word4 + 0xf7537e82, 6) + b;
    d = rotate(d + f4(a, b, c) + word11 + 0xbd3af235, 10) + a;
    c = rotate(c + f4(d, a, b) + word2 + 0x2ad7d2bb, 15) + d;
    b = rotate(b + f4(c, d, a) + word9 + 0xeb86d391, 21) + c;

    // update hash
    m_hash[0] += a;
    m_hash[1] += b;
    m_hash[2] += c;
    m_hash[3] += d;
}

/// add arbitrary number of bytes
void MD5::add(const void* data, size_t numBytes)
{
    const uint8_t* current = (const uint8_t*)data;

    if (m_bufferSize > 0) {
        while (numBytes > 0 && m_bufferSize < BlockSize) {
            m_buffer[m_bufferSize++] = *current++;
            numBytes--;
        }
    }

    // full buffer
    if (m_bufferSize == BlockSize) {
        processBlock(m_buffer);
        m_numBytes += BlockSize;
        m_bufferSize = 0;
    }

    // no more data ?
    if (numBytes == 0)
        return;

    // process full blocks
    while (numBytes >= BlockSize) {
        processBlock(current);
        current += BlockSize;
        m_numBytes += BlockSize;
        numBytes -= BlockSize;
    }

    // keep remaining bytes in buffer
    while (numBytes > 0) {
        m_buffer[m_bufferSize++] = *current++;
        numBytes--;
    }
}

/// process final block, less than 64 bytes
void MD5::processBuffer()
{
    // the input bytes are considered as bits strings, where the first bit is the most significant bit of the byte

    // - append "1" bit to message
    // - append "0" bits until message length in bit mod 512 is 448
    // - append length as 64 bit integer

    // number of bits
    size_t paddedLength = m_bufferSize * 8;

    // plus one bit set to 1 (always appended)
    paddedLength++;

    // number of bits must be (numBits % 512) = 448
    size_t lower11Bits = paddedLength & 511;
    if (lower11Bits <= 448)
        paddedLength += 448 - lower11Bits;
    else
        paddedLength += 512 + 448 - lower11Bits;
    // convert from bits to bytes
    paddedLength /= 8;

    // only needed if additional data flows over into a second block
    unsigned char extra[BlockSize];

    // append a "1" bit, 128 => binary 10000000
    if (m_bufferSize < BlockSize)
        m_buffer[m_bufferSize] = 128;
    else
        extra[0] = 128;

    size_t i;
    for (i = m_bufferSize + 1; i < BlockSize; i++)
        m_buffer[i] = 0;
    for (; i < paddedLength; i++)
        extra[i - BlockSize] = 0;

    // add message length in bits as 64 bit number
    uint64_t msgBits = 8 * (m_numBytes + m_bufferSize);
    // find right position
    unsigned char* addLength;
    if (paddedLength < BlockSize)
        addLength = m_buffer + paddedLength;
    else
        addLength = extra + paddedLength - BlockSize;

    // must be little endian
    *addLength++ = msgBits & 0xFF;
    msgBits >>= 8;
    *addLength++ = msgBits & 0xFF;
    msgBits >>= 8;
    *addLength++ = msgBits & 0xFF;
    msgBits >>= 8;
    *addLength++ = msgBits & 0xFF;
    msgBits >>= 8;
    *addLength++ = msgBits & 0xFF;
    msgBits >>= 8;
    *addLength++ = msgBits & 0xFF;
    msgBits >>= 8;
    *addLength++ = msgBits & 0xFF;
    msgBits >>= 8;
    *addLength++ = msgBits & 0xFF;

    // process blocks
    processBlock(m_buffer);
    // flowed over into a second block ?
    if (paddedLength > BlockSize)
        processBlock(extra);
}

/// return latest hash as 32 hex characters
std::string MD5::getHash()
{
    // compute hash (as raw bytes)
    unsigned char rawHash[HashBytes];
    getHash(rawHash);

    // convert to hex string
    std::string result;
    result.reserve(2 * HashBytes);
    for (int i = 0; i < HashBytes; i++) {
        static const char dec2hex[16 + 1] = "0123456789abcdef";
        result += dec2hex[(rawHash[i] >> 4) & 15];
        result += dec2hex[rawHash[i] & 15];
    }

    return result;
}

/// return latest hash as bytes
void MD5::getHash(unsigned char buffer[MD5::HashBytes])
{
    // save old hash if buffer is partially filled
    uint32_t oldHash[HashValues];
    for (int i = 0; i < HashValues; i++)
        oldHash[i] = m_hash[i];

    // process remaining bytes
    processBuffer();

    unsigned char* current = buffer;
    for (int i = 0; i < HashValues; i++) {
        *current++ = m_hash[i] & 0xFF;
        *current++ = (m_hash[i] >> 8) & 0xFF;
        *current++ = (m_hash[i] >> 16) & 0xFF;
        *current++ = (m_hash[i] >> 24) & 0xFF;

        // restore old hash
        m_hash[i] = oldHash[i];
    }
}

/// compute MD5 of a memory block
std::string MD5::operator()(const void* data, size_t numBytes)
{
    reset();
    add(data, numBytes);
    return getHash();
}

/// compute MD5 of a string, excluding final zero
std::string MD5::operator()(const std::string& text)
{
    reset();
    add(text.c_str(), text.size());
    return getHash();
}