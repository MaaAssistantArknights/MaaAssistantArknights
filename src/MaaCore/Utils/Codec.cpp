#include "Utils/Codec.h"

#include <cstdint>
#include <string>

namespace asst::utils
{
static constexpr inline void char32_to_utf8(std::string& out, char32_t ch)
{
    if (ch <= 0x7F) {
        out.push_back(static_cast<char>(ch));
    }
    else if (ch <= 0x7FF) {
        out.push_back(static_cast<char>(((ch >> 6) & 0b00011111) | 0b11000000u));
        out.push_back(static_cast<char>((ch & 0b00111111) | 0b10000000u));
    }
    else if (ch <= 0xFFFF) {
        out.push_back(static_cast<char>(((ch >> 12) & 0b00001111) | 0b11100000u));
        out.push_back(static_cast<char>(((ch >> 6) & 0b00111111) | 0b10000000u));
        out.push_back(static_cast<char>((ch & 0b00111111) | 0b10000000u));
    }
    else {
        out.push_back(static_cast<char>(((ch >> 18) & 0b00000111) | 0b11110000u));
        out.push_back(static_cast<char>(((ch >> 12) & 0b00111111) | 0b10000000u));
        out.push_back(static_cast<char>(((ch >> 6) & 0b00111111) | 0b10000000u));
        out.push_back(static_cast<char>((ch & 0b00111111) | 0b10000000u));
    }
}

static constexpr inline bool utf8_to_char32(std::string_view::iterator& cur, std::string_view::iterator end, char32_t& ch)
{
    char leading = *cur++;
    if ((leading & 0b10000000) == 0) {
        ch = leading & 0b01111111;
    }
    else if ((leading & 0b11100000) == 0b11000000) {
        if (end - cur <= 0) {
            return false;
        }
        char next = *cur++;
        ch = ((leading & 0b00011111) << 6) | (next & 0b00111111);
    }
    else if ((leading & 0b11110000) == 0b11100000) {
        if (end - cur <= 1) {
            return false;
        }
        char next1 = *cur++;
        char next2 = *cur++;
        ch = ((leading & 0b00001111) << 12) | ((next1 & 0b00111111) << 6) | (next2 & 0b00111111);
    }
    else if ((leading & 0b11111000) == 0b11110000) {
        if (end - cur <= 2) {
            return false;
        }
        char next1 = *cur++;
        char next2 = *cur++;
        char next3 = *cur++;
        ch = ((leading & 0b00001111) << 18) | ((next1 & 0b00111111) << 12) | ((next2 & 0b00111111) << 6) | (next3 & 0b00111111);
    }
    else {
        // mainly for 0b10xxxxxx, skip corrupted data
        return false;
    }
    return true;
}

static constexpr inline void char32_to_wchar(std::wstring& out, char32_t ch)
{
    if constexpr (sizeof(wchar_t) == 2) {
        if (ch <= 0xFFFF) {
            out.push_back(static_cast<wchar_t>(ch));
        }
        else {
            uint32_t delta = (ch - 0x10000) & ((1 << 20) - 1);
            out.push_back(static_cast<wchar_t>((delta >> 10) | 0xD800));
            out.push_back(static_cast<wchar_t>((delta & ((1 << 10) - 1)) | 0xDC00));
        }
    }
    else {
        out.push_back(static_cast<wchar_t>(ch));
    }
}

static constexpr inline bool wchar_to_char32(std::wstring_view::iterator& cur, std::wstring_view::iterator end, char32_t& ch)
{
    if constexpr (sizeof(wchar_t) == 2) {
        auto leading = static_cast<uint32_t>(static_cast<uint16_t>(*cur++));
        if ((leading & 0b1111'1100'0000'0000) == 0xD800) {
            if (cur == end) {
                return false;
            }
            auto next = static_cast<uint32_t>(static_cast<uint16_t>(*cur++));
            if ((next & 0b1111'1100'0000'0000) != 0xDC00) {
                return false;
            }
            ch = static_cast<char32_t>((((leading & 0b0000'0011'1111'1111) << 10) | (next & 0b0000'0011'1111'1111)) + 0x10000);
        }
        else if ((leading & 0b1111'1100'0000'0000) == 0xDC00) {
            return false;
        }
        else {
            ch = static_cast<wchar_t>(leading & 0b1111'1111'1111'1111);
        }
        return true;
    }
    else {
        ch = static_cast<char32_t>(*cur++);
        return true;
    }
}

std::wstring to_u16(std::string_view u8str)
{
    auto cur = u8str.begin();
    char32_t ch;
    std::wstring output;
    while (cur != u8str.end()) {
        if (!utf8_to_char32(cur, u8str.end(), ch)) {
            continue;
        }
        char32_to_wchar(output, ch);
    }
    return output;
}

std::string from_u16(std::wstring_view u16str)
{
    auto cur = u16str.begin();
    char32_t ch;
    std::string output;
    while (cur != u16str.end()) {
        if (!wchar_to_char32(cur, u16str.end(), ch)) {
            continue;
        }
        char32_to_utf8(output, ch);
    }
    return output;
}
}