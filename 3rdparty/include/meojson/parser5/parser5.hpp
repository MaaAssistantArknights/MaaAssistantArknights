#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stack>
#include <vector>

#include "../common/types.hpp"
#include "unicode.h"

namespace json
{
// *************************
// *         declare       *
// *************************

template <typename string_t = default_string_t>
class parser5
{
private:
    using string_iter_t = typename string_t::const_iterator;
    using u8char = uint64_t;

    /* exceptions */
public:
    class exception : public json::exception
    {
    public:
        exception() = default;
        exception(const std::string& type, const std::string& msg, const std::string& detail)
        {
            std::stringstream ss;
            ss << "JSON5: [" << type << "] " << msg << '\n';
            ss << detail << std::endl;
            _what = ss.str();
        }
        exception(const exception&) = default;
        exception& operator=(const exception&) = default;
        exception(exception&&) = default;
        exception& operator=(exception&&) = default;

        virtual ~exception() noexcept = default;
    };

    class InvalidChar : public exception
    {
    public:
        InvalidChar(u8char ch = 0, const std::string& detail = "")
            : exception("Invalid Char", "Unexpected token \'" + StringFromCharCode(ch) + "\'", detail)
        {}
    };

    class InvalidIdentifier : public exception
    {
    public:
        InvalidIdentifier(const std::string& msg = "", const std::string& detail = "")
            : exception("Invalid Identifier", msg, detail)
        {}
    };

    class InvalidEOF : public exception
    {
    public:
        InvalidEOF(const std::string& msg = "", const std::string& detail = "") : exception("Invalid EOF", msg, detail)
        {}
    };

public:
    std::string exceptionDetailInfo()
    {
        size_t start_point = _col;
        auto len = _print_len + 1;
        auto current_line = _line_begin_cur;
        std::stringstream ss;
        ss << "at line " << _line << ", column " << _col + 1 << '\n';

        while (read() != '\n' && _cur != _end && (_col - start_point) < 5)
            ;
        ss << std::string(current_line, _cur) << '\n';
        ss << std::setw(len) << '^' << '\n';

#ifdef __DEBUG_JSON__
        ss << "[DEBUG] current lex state: " << static_cast<int>(_lex_state) << '\n';
        ss << "[DEBUG] current parse state: " << static_cast<int>(_parse_state) << '\n';
#endif
        return ss.str();
    }

    /* helper class and type */
private:
    class unicode
    {
    public:
        static bool isSpaceSeparator(u8char ch);
        static bool isIdStartChar(u8char ch);
        static bool isIdContinueChar(u8char ch);
        static bool isDigit(u8char ch);
        static bool isHexDigit(u8char ch);
        static u8char toUnicode(u8char ch);
        template <typename array_t>
        static bool findInRange(const array_t& range, /* unicode code point */ u8char codePoint);
    };

    enum class LexState
    {
        default_ = 0,
        comment,
        multiLineComment,
        multiLineCommentAsterisk,
        singleLineComment,
        value,
        identifierNameStartEscape,
        identifierName,
        identifierNameEscape,
        sign,
        zero,
        decimalInteger,
        decimalPointLeading,
        decimalPoint,
        decimalFraction,
        decimalExponent,
        decimalExponentSign,
        decimalExponentInteger,
        hexadecimal,
        hexadecimalInteger,
        string,
        start,
        beforePropertyName,
        afterPropertyName,
        beforePropertyValue,
        afterPropertyValue,
        beforeArrayValue,
        afterArrayValue,
        end
    };

    enum class ParseState
    {
        start = 21,
        beforePropertyName,
        afterPropertyName,
        beforePropertyValue,
        afterPropertyValue,
        beforeArrayValue,
        afterArrayValue,
        end
    };

    enum class TokenType
    {
        eof = 0,
        punctuator,
        identifier,
        null,
        boolean,
        numeric,
        string,
    };

    struct Token
    {
        TokenType type = TokenType::eof;
        value _value;
        size_t col = 0;
        size_t line = 0;
    };

    /* constrators and callers */
public:
    ~parser5() noexcept = default;
    static std::optional<value> parse(const string_t& content, std::string* error = nullptr);

private:
    parser5(string_iter_t cbegin, string_iter_t cend) noexcept : _cur(cbegin), _end(cend), _line_begin_cur(cbegin) {}
    std::optional<value> parse();

private:
    /* utf-8 reader */
    static u8char peek(const string_iter_t& begin, const string_iter_t& end, size_t* len = nullptr);
    static u8char peek(const std::string& str);
    typename parser5<string_t>::u8char read();
    static std::string StringFromCharCode(u8char code);
    /* escape and format */
    void literal(const std::string& s);
    std::optional<u8char> escape();
    u8char hexEscape();
    u8char unicodeEscape();
    /* lex, parse, token */
    Token lex();
    Token newToken(TokenType type, value value);

    std::optional<Token> lex_default();
    std::optional<Token> lex_comment();
    std::optional<Token> lex_multiLineComment();
    std::optional<Token> lex_multiLineCommentAsterisk();
    std::optional<Token> lex_singleLineComment();
    std::optional<Token> lex_value();
    std::optional<Token> lex_identifierNameStartEscape();
    std::optional<Token> lex_identifierName();
    std::optional<Token> lex_identifierNameEscape();
    std::optional<Token> lex_sign();
    std::optional<Token> lex_zero();
    std::optional<Token> lex_decimalInteger();
    std::optional<Token> lex_decimalPointLeading();
    std::optional<Token> lex_decimalPoint();
    std::optional<Token> lex_decimalFraction();
    std::optional<Token> lex_decimalExponent();
    std::optional<Token> lex_decimalExponentSign();
    std::optional<Token> lex_decimalExponentInteger();
    std::optional<Token> lex_hexadecimal();
    std::optional<Token> lex_hexdecimalInteger();
    std::optional<Token> lex_string();
    std::optional<Token> lex_start();
    std::optional<Token> lex_beforePropertyName();
    std::optional<Token> lex_afterPropertyName();
    std::optional<Token> lex_beforePropertyValue();
    std::optional<Token> lex_afterPropertyValue();
    std::optional<Token> lex_beforeArrayValue();
    std::optional<Token> lex_afterArrayValue();
    std::optional<Token> lex_end();

    std::optional<Token> lexStates(LexState state);

    void parse_start();
    void parse_beforePropertyName();
    void parse_afterPropertyName();
    void parse_beforePropertyValue();
    void parse_beforeArrayValue();
    void parse_afterPropertyValue();
    void parse_afterArrayValue();
    void parse_end();

    void parseStates(ParseState state);

    /* stack operation */
    void push();
    void pop();

private:
    string_iter_t _cur, _end;
    string_iter_t _line_begin_cur;
    size_t _line = 1, _col = 0, _print_len = 0;
    ParseState _parse_state = ParseState::start;
    std::stack<value*> _stack;
    std::optional<Token> _token;
    std::string _key;
    std::optional<value> _root;

    LexState _lex_state = LexState::default_;
    std::string _buffer;
    bool _double_quote = false;
    int64_t _sign = 1;
    u8char _current_char = 0;
};

// *************************
// *    implementation     *
// *************************

/* parser5<string_t>::unicode */

template <typename string_t>
inline bool parser5<string_t>::unicode::isSpaceSeparator(u8char ch)
{
    return findInRange(json::unicode::space_separator, toUnicode(ch));
}

template <typename string_t>
inline bool parser5<string_t>::unicode::isIdStartChar(u8char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '$') || (ch == '_') ||
           findInRange(json::unicode::id_start, toUnicode(ch));
}

template <typename string_t>
inline bool parser5<string_t>::unicode::isIdContinueChar(u8char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || (ch == '$') ||
           (ch == '_') || findInRange(json::unicode::id_continue, toUnicode(ch));
}

template <typename string_t>
inline bool parser5<string_t>::unicode::isDigit(u8char ch)
{
    auto str = StringFromCharCode(ch);
    return std::regex_search(str, std::regex(R"([0-9])"));
}

template <typename string_t>
inline bool parser5<string_t>::unicode::isHexDigit(u8char ch)
{
    auto str = StringFromCharCode(ch);
    return std::regex_search(str, std::regex(R"([0-9A-Fa-f])"));
}

template <typename string_t>
inline uint64_t parser5<string_t>::unicode::toUnicode(u8char ch)
{
    std::stack<uint8_t> coded;
    if (ch == 0) {
        return ch;
    }
    while (ch > 0) {
        coded.push(ch & 0xff);
        ch = ch >> 8;
    }
    u8char charcode = 0;
    uint8_t t = coded.top();
    coded.pop();
    if (t < 128) {
        return t;
    }
    uint8_t high_bit_mask = (1 << 6) - 1;
    uint8_t high_bit_shift = 0;
    int total_bits = 0;
    const int other_bits = 6;
    while ((t & 0xC0) == 0xC0) {
        t <<= 1;
        t &= 0xff;
        total_bits += 6;
        high_bit_mask >>= 1;
        high_bit_shift++;
        charcode <<= other_bits;
        charcode |= coded.top() & ((1 << other_bits) - 1);
        coded.pop();
    }
    charcode |= static_cast<uint64_t>((t >> high_bit_shift) & high_bit_mask) << total_bits;
    return charcode;
}

template <typename string_t>
template <typename array_t>
inline bool parser5<string_t>::unicode::findInRange(const array_t& range, u8char codePoint)
{
    const auto begin = std::begin(range);
    const auto end = std::end(range);

    const auto [lb, ub] = std::equal_range(begin, end, codePoint);

    // 如果 lb == ub, 则表示codePoint不在range中
    if (lb == ub) {
        return false;
    }
    // set 中保存的是类似于 { start1, end1, start2, end2, ... } 的形式, 区间可表示为[start, end)
    // 判断lb是否位于start的位置, 如果是, 则表示codePoint在某个区间的内部, 如果不是, 则表示codePoint在两个区间中间
    return std::distance(begin, lb) % 2 == 0;
}

/* constrators and callers */
template <typename string_t>
std::optional<value> parse5(const string_t& content, std::string* error = nullptr)
{
    return parser5<string_t>::parse(content, error);
}

/* Wrapper for handling input of C-style strings. */
template <typename CharT>
std::optional<value> parse5(CharT* content, std::string* error = nullptr)
{
    return parse5(std::basic_string_view<std::decay_t<CharT>> { content }, error);
}

template <typename string_t>
inline std::optional<value> parser5<string_t>::parse(const string_t& content, std::string* error)
{
    try {
        return parser5<string_t>(content.cbegin(), content.cend()).parse();
    }
    catch (json::parser5<string_t>::exception& ex) {
        if (error) {
            *error = ex.what();
        }
    }
    return std::nullopt;
}

template <typename string_t>
inline std::optional<value> parser5<string_t>::parse()
{
    do {
        _token = lex();
        parseStates(_parse_state);
    } while (_token->type != TokenType::eof);

    return _root;
}

/* escape and format */
template <typename string_t>
inline void parser5<string_t>::literal(const std::string& s)
{
    for (const auto& ch : s) {
        char p = static_cast<char>(read());
        if (p != ch) {
            throw InvalidChar(_current_char, exceptionDetailInfo());
        }
    }
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::u8char> parser5<string_t>::escape()
{
    auto c = peek(_cur, _end);
    switch (c) {
    case 'b':
        read();
        return '\b';

    case 'f':
        read();
        return '\f';

    case 'n':
        read();
        return '\n';

    case 'r':
        read();
        return '\r';

    case 't':
        read();
        return '\t';

    case 'v':
        read();
        return '\v';

    case '0':
        read();
        if (unicode::isDigit(peek(_cur, _end))) {
            throw InvalidChar(_current_char, exceptionDetailInfo());
        }

        return '\0';

    case 'x':
        read();
        return hexEscape();

    case 'u':
        read();
        return unicodeEscape();

    case '\n':
    case 0x2028:
    case 0x2029:
        read();
        return 0;

    case '\r':
        read();
        if (peek(_cur, _end) == '\n') {
            read();
        }

        return std::nullopt;

    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        throw InvalidChar(_current_char, exceptionDetailInfo());
    default:
        if (c == 0) {
            throw InvalidChar(_current_char, exceptionDetailInfo());
        }
    }

    return read();
}

template <typename string_t>
inline typename parser5<string_t>::u8char parser5<string_t>::hexEscape()
{
    std::string buffer = "";
    auto c = peek(_cur, _end);

    if (!unicode::isHexDigit(c)) {
        throw InvalidChar(_current_char, exceptionDetailInfo());
    }

    buffer += static_cast<char>(read());

    c = peek(_cur, _end);
    if (!unicode::isHexDigit(c)) {
        throw InvalidChar(_current_char, exceptionDetailInfo());
    }

    buffer += static_cast<char>(read());

    return std::stoi(buffer, nullptr, 16);
}

template <typename string_t>
inline typename parser5<string_t>::u8char parser5<string_t>::unicodeEscape()
{
    std::string buffer;
    int count = 4;
    while (count-- > 0) {
        auto c = peek(_cur, _end);
        if (!unicode::isHexDigit(c)) {
            throw InvalidChar(_current_char, exceptionDetailInfo());
        }
        buffer += StringFromCharCode(read());
    }

    return std::stoull(buffer, nullptr, 16);
}

/* utf-8 reader */
template <typename string_t>
inline typename parser5<string_t>::u8char parser5<string_t>::peek(const string_iter_t& begin, const string_iter_t& end,
                                                                  size_t* plen)
{
    if (begin == end) {
        if (plen) *plen = 0;
        return 0;
    }
    uint8_t head = *begin;
    uint64_t ch = head;
    size_t len = 1;
    while ((head & 0b11000000) > 0b10000000) {
        head <<= 1;
        ++len;
        ch <<= 8;
        auto sep = begin + len - 1;
        if (sep < end) {
            ch += (uint8_t)(*sep);
        }
    }
    if (plen) {
        *plen = len;
    }
    return ch;
}

template <typename string_t>
inline typename parser5<string_t>::u8char parser5<string_t>::peek(const std::string& str)
{
    return peek(str.cbegin(), str.cend());
}
inline constexpr size_t operator"" _sz(unsigned long long size)
{
    return size;
}
template <typename string_t>
inline typename parser5<string_t>::u8char parser5<string_t>::read()
{
    size_t len = 0;
    _current_char = peek(_cur, _end, &len);
    if (_current_char == '\n') {
        _line++;
        _col = 0;
        _print_len = 0;
        _line_begin_cur = _cur + 1;
    }
    else if (_current_char > 0) {
        _col++;
    }

    if (len > 0) {
        _print_len += (std::min)(len, 2_sz);
        _cur += len;
        if (_cur > _end) {
            _cur = _end;
        }
    }
    return _current_char;
}

template <typename string_t>
inline std::string parser5<string_t>::StringFromCharCode(typename parser5<string_t>::u8char code)
{
    if (code == 0) return "";
    std::string str;
    for (auto i = 0; i < 8; ++i) {
        auto ch = (0xff & code);
        if (ch) str.insert(0, 1, static_cast<char>(ch));
        code >>= 8;
    }
    return str;
}

/* lex, parse, token */

template <typename string_t>
inline typename parser5<string_t>::Token parser5<string_t>::newToken(TokenType type, value value)
{
    Token token {};
    token.type = type;
    token._value = value;
    token.line = _line;
    token.col = _col;
    return token;
}

template <typename string_t>
inline typename parser5<string_t>::Token parser5<string_t>::lex()
{
    _lex_state = LexState::default_;
    _buffer = "";
    _double_quote = false;
    _sign = 1;

    std::optional<Token> token;

    while (_cur != _end) {
        _current_char = peek(_cur, _end);

        token = lexStates(_lex_state);
        if (token.has_value()) {
            return token.value();
        }
    }
    return Token { TokenType::eof, value(), _line, _col };
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_default()
{
    switch (_current_char) {
    case '\t':
    case '\v':
    case '\f':
    case ' ':
    case '\n':
    case '\r':
    case 0x00A0:
    case 0xFEFF:
    case 0x2028:
    case 0x2029:
        read();
        return std::nullopt;
    case '/':
        read();
        _lex_state = LexState::comment;
        return std::nullopt;
    case 0:
        read();
        return newToken(TokenType::eof, value());
    }

    // if (_current_char.size() == 0) {
    //   read();
    //   return newToken("eof");
    // }

    if (unicode::isSpaceSeparator(_current_char)) {
        read();
        return std::nullopt;
    }

    //      throw InvalidLexState();
    return lexStates((LexState)_parse_state);
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_comment()
{
    switch (_current_char) {
    case '*':
        read();
        _lex_state = LexState::multiLineComment;
        return std::nullopt;

    case '/':
        read();
        _lex_state = LexState::singleLineComment;
        return std::nullopt;
    }

    throw InvalidChar(_current_char, exceptionDetailInfo());
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_multiLineComment()
{
    if (_current_char == '*') {
        read();
        _lex_state = LexState::multiLineCommentAsterisk;
        return std::nullopt;
    }

    if (_current_char == 0) {
        throw InvalidChar(_current_char, exceptionDetailInfo());
    }

    read();
    return std::nullopt;
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_multiLineCommentAsterisk()
{
    switch (_current_char) {
    case '*':
        read();
        return std::nullopt;

    case '/':
        read();
        _lex_state = LexState::default_;
        return std::nullopt; //$

    default:
        if (_current_char == 0) {
            throw InvalidChar(_current_char, exceptionDetailInfo());
        }
    }

    read();
    _lex_state = LexState::multiLineComment;
    return std::nullopt;
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_singleLineComment()
{
    switch (_current_char) {
    case '\n':
    case '\r':
    case 0x2028:
    case 0x2029:
        read();
        _lex_state = LexState::default_;
        return std::nullopt;
    case 0:
        read();
        return newToken(TokenType::eof, value());
    }
    read();
    return std::nullopt;
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_value()
{
    switch (_current_char) {
    case '{':
    case '[':
        return newToken(TokenType::punctuator, value(StringFromCharCode(read())));
    case 'n':
        read();
        literal("ull");
        return newToken(TokenType::null, value());
    case 't':
        read();
        literal("rue");
        return newToken(TokenType::boolean, value(true));
    case 'f':
        read();
        literal("alse");
        return newToken(TokenType::boolean, value(false));
    case '-':
    case '+':
        if (read() == '-') {
            _sign = -1;
        }
        _lex_state = LexState::sign;
        return std::nullopt;
    case '.':
        _buffer = StringFromCharCode(read());
        _lex_state = LexState::decimalPointLeading;
        return std::nullopt;

    case '0':
        _buffer = StringFromCharCode(read());
        _lex_state = LexState::zero;
        return std::nullopt;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        _buffer = StringFromCharCode(read());
        _lex_state = LexState::decimalInteger;
        return std::nullopt;
    case 'I':
        read();
        literal("nfinity");
        return newToken(TokenType::numeric, INFINITY);
    case 'N':
        read();
        literal("aN");
        return newToken(TokenType::numeric, NAN);
    case '\"':
    case '\'':
        _double_quote = (read() == '\"');
        _buffer = "";
        _lex_state = LexState::string;
        return std::nullopt;
    }
    throw InvalidChar(_current_char, exceptionDetailInfo());
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_identifierNameStartEscape()
{
    if (_current_char != 'u') {
        throw InvalidChar(_current_char, exceptionDetailInfo());
    }
    read();
    auto u = unicodeEscape();
    switch (u) {
    case '$':
    case '_':
        break;
    default:
        if (!unicode::isIdStartChar(u)) {
            throw InvalidIdentifier("", exceptionDetailInfo());
        }
        break;
    }
    _buffer += StringFromCharCode(u);
    _lex_state = LexState::identifierName;
    return std::nullopt;
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_identifierName()
{
    switch (_current_char) {
    case '$':
    case '_':
    case 0x200C:
    case 0x200D:
        _buffer += StringFromCharCode(read());
        return std::nullopt;
    case '\\':
        read();
        _lex_state = LexState::identifierNameEscape;
        return std::nullopt;
    }

    if (unicode::isIdContinueChar(_current_char)) {
        _buffer += StringFromCharCode(read());
        return std::nullopt;
    }

    return newToken(TokenType::identifier, _buffer);
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_identifierNameEscape()
{
    if (_current_char != 'u') {
        throw InvalidChar(_current_char, exceptionDetailInfo());
    }
    read();
    auto u = unicodeEscape();
    switch (u) {
    case '$':
    case '_':
    case 0x200C:
    case 0x200D:
        break;
    default:
        if (!unicode::isIdStartChar(u)) {
            throw InvalidIdentifier("", exceptionDetailInfo());
        }
        break;
    }
    _buffer += StringFromCharCode(u);
    _lex_state = LexState::identifierName;
    return std::nullopt;
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_sign()
{
    switch (_current_char) {
    case '.':
        _buffer = StringFromCharCode(read());
        _lex_state = LexState::decimalPointLeading;
        return std::nullopt;

    case '0':
        _buffer = StringFromCharCode(read());
        _lex_state = LexState::zero;
        return std::nullopt;

    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        _buffer = StringFromCharCode(read());
        _lex_state = LexState::decimalInteger;
        return std::nullopt;

    case 'I':
        read();
        literal("nfinity");
        return newToken(TokenType::numeric, _sign * INFINITY);

    case 'N':
        read();
        literal("aN");
        return newToken(TokenType::numeric, NAN);
    }
    throw InvalidChar(_current_char, exceptionDetailInfo());
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_zero()
{
    switch (_current_char) {
    case '.':
        _buffer += StringFromCharCode(read());
        _lex_state = LexState::decimalPoint;
        return std::nullopt; //$

    case 'e':
    case 'E':
        _buffer += StringFromCharCode(read());
        _lex_state = LexState::decimalExponent;
        return std::nullopt; //$

    case 'x':
    case 'X':
        _buffer += StringFromCharCode(read());
        _lex_state = LexState::hexadecimal;
        return std::nullopt; //$
    }
    return newToken(TokenType::numeric, _sign * 0);
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_decimalInteger()
{
    switch (_current_char) {
    case '.':
        _buffer += StringFromCharCode(read());
        _lex_state = LexState::decimalPoint;
        return std::nullopt;
    case 'e':
    case 'E':
        _buffer += StringFromCharCode(read());
        _lex_state = LexState::decimalExponent;
        return std::nullopt; //$
    }
    if (unicode::isDigit(_current_char)) {
        _buffer += StringFromCharCode(read());
        return std::nullopt;
    }

    std::string number = _sign == -1 ? ("-" + _buffer) : _buffer;
    return newToken(TokenType::numeric, value(value::value_type::number, std::move(number)));
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_decimalPointLeading()
{
    if (unicode::isDigit(_current_char)) {
        _buffer += StringFromCharCode(read());
        _lex_state = LexState::decimalFraction;
        return std::nullopt;
    }
    throw InvalidChar(_current_char, exceptionDetailInfo());
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_decimalPoint()
{
    switch (_current_char) {
    case 'e':
    case 'E':
        _buffer += StringFromCharCode(read());
        _lex_state = LexState::decimalExponent;
        return std::nullopt; //$
    }
    if (unicode::isDigit(_current_char)) {
        _buffer += StringFromCharCode(read());
        _lex_state = LexState::decimalFraction;
        return std::nullopt;
    }

    std::string number = _sign == -1 ? ("-" + _buffer) : _buffer;
    return newToken(TokenType::numeric, value(value::value_type::number, std::move(number)));
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_decimalFraction()
{
    switch (_current_char) {
    case 'e':
    case 'E':
        _buffer += StringFromCharCode(read());
        _lex_state = LexState::decimalExponent;
        return std::nullopt; //$
    }
    if (unicode::isDigit(_current_char)) {
        _buffer += StringFromCharCode(read());
        return std::nullopt;
    }

    std::string number = _sign == -1 ? ("-" + _buffer) : _buffer;
    return newToken(TokenType::numeric, value(value::value_type::number, std::move(number)));
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_decimalExponent()
{
    switch (_current_char) {
    case '+':
    case '-':
        _buffer += StringFromCharCode(read());
        _lex_state = LexState::decimalExponentSign;
        return std::nullopt; //$
    }

    if (unicode::isDigit(_current_char)) {
        _buffer += StringFromCharCode(read());
        _lex_state = LexState::decimalExponentInteger;
        return std::nullopt;
    }
    throw InvalidChar(_current_char, exceptionDetailInfo());
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_decimalExponentSign()
{
    if (unicode::isDigit(_current_char)) {
        _buffer += StringFromCharCode(read());
        _lex_state = LexState::decimalExponentInteger;
        return std::nullopt;
    }
    throw InvalidChar(_current_char, exceptionDetailInfo());
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_decimalExponentInteger()
{
    if (unicode::isDigit(_current_char)) {
        _buffer += StringFromCharCode(read());
        return std::nullopt;
    }

    std::string number = _sign == -1 ? ("-" + _buffer) : _buffer;
    return newToken(TokenType::numeric, value(value::value_type::number, std::move(number)));
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_hexadecimal()
{
    if (unicode::isHexDigit(_current_char)) {
        _buffer += StringFromCharCode(read());
        _lex_state = LexState::hexadecimalInteger;
        return std::nullopt;
    }
    throw InvalidChar(_current_char, exceptionDetailInfo());
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_hexdecimalInteger()
{
    if (unicode::isHexDigit(_current_char)) {
        _buffer += StringFromCharCode(read());
        return std::nullopt;
    }

    std::string number = _sign == -1 ? ("-" + _buffer) : _buffer;
    return newToken(TokenType::numeric, value(value::value_type::number, std::move(number)));
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_string()
{
    switch (_current_char) {
    case '\\':
        read();
        _buffer += StringFromCharCode(escape().value_or(0));
        return std::nullopt;
    case '\"':
        if (_double_quote) {
            read();
            return newToken(TokenType::string, _buffer);
        }
        _buffer += StringFromCharCode(read());
        return std::nullopt;
    case '\'':
        if (!_double_quote) {
            read();
            return newToken(TokenType::string, _buffer);
        }
        _buffer += StringFromCharCode(read());
        return std::nullopt;
    case '\n':
    case '\r':
        throw InvalidChar(_current_char, exceptionDetailInfo());
    case 0x2028:
    case 0x2029:
        // throw separatorChar(_current_char);
        break;
    default:
        if (_current_char == 0) {
            throw InvalidChar(_current_char, exceptionDetailInfo());
        }
    }
    _buffer += StringFromCharCode(read());
    return std::nullopt;
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_start()
{
    switch (_current_char) {
    case '{':
    case '[':
        return newToken(TokenType::punctuator, StringFromCharCode(read()));
    }

    _lex_state = LexState::value;
    return std::nullopt;
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_beforePropertyName()
{
    switch (_current_char) {
    case '$':
    case '_':
        _buffer = StringFromCharCode(read());
        _lex_state = LexState::identifierName;
        return std::nullopt;
    case '\\':
        read();
        _lex_state = LexState::identifierNameStartEscape;
        return std::nullopt;
    case '}':
        return newToken(TokenType::punctuator, StringFromCharCode(read()));
    case '\"':
    case '\'':
        _double_quote = (read() == '\"');
        _lex_state = LexState::string;
        return std::nullopt;
    }

    if (unicode::isIdStartChar(_current_char)) {
        _buffer += StringFromCharCode(read());
        _lex_state = LexState::identifierName;
        return std::nullopt;
    }

    throw InvalidChar(_current_char, exceptionDetailInfo());
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_afterPropertyName()
{
    if (_current_char == ':') {
        return newToken(TokenType::punctuator, StringFromCharCode(read()));
    }
    throw InvalidChar(_current_char, exceptionDetailInfo());
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_beforePropertyValue()
{
    _lex_state = LexState::value;
    return std::nullopt;
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_afterPropertyValue()
{
    switch (_current_char) {
    case ',':
    case '}':
        return newToken(TokenType::punctuator, StringFromCharCode(read()));
    }
    throw InvalidChar(_current_char, exceptionDetailInfo());
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_beforeArrayValue()
{
    if (_current_char == ']') {
        return newToken(TokenType::punctuator, StringFromCharCode(read()));
    }
    _lex_state = LexState::value;
    return std::nullopt;
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_afterArrayValue()
{
    switch (_current_char) {
    case ',':
    case ']':
        return newToken(TokenType::punctuator, StringFromCharCode(read()));
    }

    throw InvalidChar(_current_char, exceptionDetailInfo());
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lex_end()
{
    throw InvalidChar(_current_char, exceptionDetailInfo());
}

template <typename string_t>
inline std::optional<typename parser5<string_t>::Token> parser5<string_t>::lexStates(LexState state)
{
    switch (state) {
    case LexState::default_:
        return lex_default();
    case LexState::comment:
        return lex_comment();
    case LexState::multiLineComment:
        return lex_multiLineComment();
    case LexState::multiLineCommentAsterisk:
        return lex_multiLineCommentAsterisk();
    case LexState::singleLineComment:
        return lex_singleLineComment();
    case LexState::value:
        return lex_value();
    case LexState::identifierNameStartEscape:
        return lex_identifierNameStartEscape();
    case LexState::identifierName:
        return lex_identifierName();
    case LexState::identifierNameEscape:
        return lex_identifierNameEscape();
    case LexState::sign:
        return lex_sign();
    case LexState::zero:
        return lex_zero();
    case LexState::decimalInteger:
        return lex_decimalInteger();
    case LexState::decimalPointLeading:
        return lex_decimalPointLeading();
    case LexState::decimalPoint:
        return lex_decimalPoint();
    case LexState::decimalFraction:
        return lex_decimalFraction();
    case LexState::decimalExponent:
        return lex_decimalExponent();
    case LexState::decimalExponentSign:
        return lex_decimalExponentSign();
    case LexState::decimalExponentInteger:
        return lex_decimalExponentInteger();
    case LexState::hexadecimal:
        return lex_hexadecimal();
    case LexState::hexadecimalInteger:
        return lex_hexdecimalInteger();
    case LexState::string:
        return lex_string();
    case LexState::start:
        return lex_start();
    case LexState::beforePropertyName:
        return lex_beforePropertyName();
    case LexState::afterPropertyName:
        return lex_afterPropertyName();
    case LexState::beforePropertyValue:
        return lex_beforePropertyValue();
    case LexState::afterPropertyValue:
        return lex_afterPropertyValue();
    case LexState::beforeArrayValue:
        return lex_beforeArrayValue();
    case LexState::afterArrayValue:
        return lex_afterArrayValue();
    case LexState::end:
        return lex_end();
    }

    // throw
    return std::nullopt;
}

template <typename string_t>
inline void parser5<string_t>::parse_start()
{
    if (_token->type == TokenType::eof) {
        throw InvalidEOF("EOF", exceptionDetailInfo());
    }

    push();
}

template <typename string_t>
inline void parser5<string_t>::parse_beforePropertyName()
{
    switch (_token->type) {
    case TokenType::identifier:
    case TokenType::string:
        _key = _token->_value.as_string();
        _parse_state = ParseState::afterPropertyName;
        break;
    case TokenType::punctuator:
        pop();
        break;
    case TokenType::eof:
        throw InvalidEOF("EOF", exceptionDetailInfo());
        break;
    default:

        break;
    }
}

template <typename string_t>
inline void parser5<string_t>::parse_afterPropertyName()
{
    if (_token->type == TokenType::eof) {
        throw InvalidEOF("EOF", exceptionDetailInfo());
    }

    _parse_state = ParseState::beforePropertyValue;
}

template <typename string_t>
inline void parser5<string_t>::parse_beforePropertyValue()
{
    if (_token->type == TokenType::eof) {
        throw InvalidEOF("EOF", exceptionDetailInfo());
    }
    push();
}

template <typename string_t>
inline void parser5<string_t>::parse_beforeArrayValue()
{
    if (_token->type == TokenType::eof) {
        throw InvalidEOF("EOF", exceptionDetailInfo());
    }

    if (_token->type == TokenType::punctuator && _token->_value.as_string()[0] == ']') {
        pop();
        return;
    }

    push();
}

template <typename string_t>
inline void parser5<string_t>::parse_afterPropertyValue()
{
    if (_token->type == TokenType::eof) {
        throw InvalidEOF("EOF", exceptionDetailInfo());
    }

    switch (_token->_value.as_string()[0]) {
    case ',':
        _parse_state = ParseState::beforePropertyName;
        break;
    case '}':
        pop();
        break;
    }
}

template <typename string_t>
inline void parser5<string_t>::parse_afterArrayValue()
{
    if (_token->type == TokenType::eof) {
        throw InvalidEOF("EOF", exceptionDetailInfo());
    }
    switch (_token->_value.as_string()[0]) {
    case ',':
        _parse_state = ParseState::beforeArrayValue;
        break;
    case ']':
        pop();
        break;
    }
}

template <typename string_t>
inline void parser5<string_t>::parse_end()
{}

template <typename string_t>
inline void parser5<string_t>::parseStates(ParseState state)
{
    switch (state) {
    case ParseState::start:
        parse_start();
        break;
    case ParseState::beforePropertyName:
        parse_beforePropertyName();
        break;
    case ParseState::afterPropertyName:
        parse_afterPropertyName();
        break;
    case ParseState::beforePropertyValue:
        parse_beforePropertyValue();
        break;
    case ParseState::beforeArrayValue:
        parse_beforeArrayValue();
        break;
    case ParseState::afterPropertyValue:
        parse_afterPropertyValue();
        break;
    case ParseState::afterArrayValue:
        parse_afterArrayValue();
        break;
    default:
        break;
    }
}
/* stack operation */
template <typename string_t>
inline void parser5<string_t>::push()
{
    value v;
    value* pv = nullptr; // only for access
    switch (_token->type) {
    case TokenType::punctuator: {
        switch (_token->_value.as_string()[0]) {
        case '{':
            v = object();
            break;
        case '[':
            v = array();
            break;
        default:
            break;
        }
    } break;
    case TokenType::null:
    case TokenType::boolean:
    case TokenType::numeric:
    case TokenType::string:
        std::swap(v, _token->_value);
        break;
    default:
        break;
    }

    if (!_root.has_value()) {
        _root = std::move(v);
        pv = &_root.value();
    }
    else {
        auto parent = _stack.top();
        if (parent->is_array()) {
            parent->as_array().emplace_back(std::move(v));
            pv = &parent->as_array()[parent->as_array().size() - 1];
        }
        else {
            parent->as_object()[_key] = std::move(v);
            pv = &parent->as_object()[_key];
        }
    }

    if (pv->is_object() || pv->is_array()) {
        _stack.emplace(pv);
        if (pv->is_array()) {
            _parse_state = ParseState::beforeArrayValue;
        }
        else {
            _parse_state = ParseState::beforePropertyName;
        }
    }
    else {
        if (_stack.empty()) {
            _parse_state = ParseState::end;
        }
        else if (_stack.top()->is_array()) {
            _parse_state = ParseState::afterArrayValue;
        }
        else {
            _parse_state = ParseState::afterPropertyValue;
        }
    }
}

template <typename string_t>
inline void parser5<string_t>::pop()
{
    _stack.pop();

    if (_stack.empty()) {
        _parse_state = ParseState::end;
    }
    else if (_stack.top()->is_array()) {
        _parse_state = ParseState::afterArrayValue;
    }
    else {
        _parse_state = ParseState::afterPropertyValue;
    }
}
} // namespace json
