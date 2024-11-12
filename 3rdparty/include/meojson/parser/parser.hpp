#pragma once

#include <fstream>
#include <optional>
#include <ostream>
#include <string>
#include <tuple>

#include "../common/types.hpp"
#include "packed_bytes.hpp"

namespace json
{
// ****************************
// *      parser declare      *
// ****************************

template <typename string_t = default_string_t, typename parsing_t = void,
          typename accel_traits = _packed_bytes::packed_bytes_trait_max>
class parser
{
public:
    using parsing_iter_t = typename parsing_t::const_iterator;

public:
    ~parser() noexcept = default;

    static std::optional<basic_value<string_t>> parse(const parsing_t& content);

private:
    parser(parsing_iter_t cbegin, parsing_iter_t cend) noexcept : _cur(cbegin), _end(cend) { ; }

    std::optional<basic_value<string_t>> parse();
    basic_value<string_t> parse_value();

    basic_value<string_t> parse_null();
    basic_value<string_t> parse_boolean();
    basic_value<string_t> parse_number();
    // parse and return a basic_value<string_t> whose type is value_type::string
    basic_value<string_t> parse_string();
    basic_value<string_t> parse_array();
    basic_value<string_t> parse_object();

    // parse and return a string_t
    std::optional<string_t> parse_stdstring();

    bool skip_string_literal_with_accel();
    bool skip_whitespace() noexcept;
    bool skip_digit();

private:
    parsing_iter_t _cur;
    parsing_iter_t _end;
};

// ***************************
// *      utils declare      *
// ***************************

template <typename parsing_t>
auto parse(const parsing_t& content);

template <typename char_t>
auto parse(char_t* content);

template <typename istream_t,
          typename = std::enable_if_t<std::is_base_of_v<std::basic_istream<typename istream_t::char_type>, istream_t>>>
auto parse(istream_t& istream, bool check_bom);

template <typename ifstream_t = std::ifstream, typename path_t = void>
auto open(const path_t& path, bool check_bom = false);

namespace literals
{
    value operator""_json(const char* str, size_t len);
    wvalue operator""_json(const wchar_t* str, size_t len);

    value operator""_jvalue(const char* str, size_t len);
    wvalue operator""_jvalue(const wchar_t* str, size_t len);

    array operator""_jarray(const char* str, size_t len);
    warray operator""_jarray(const wchar_t* str, size_t len);

    object operator""_jobject(const char* str, size_t len);
    wobject operator""_jobject(const wchar_t* str, size_t len);
}

template <typename string_t = default_string_t>
const basic_value<string_t> invalid_value();

// *************************
// *      parser impl      *
// *************************

template <typename string_t, typename parsing_t, typename accel_traits>
inline std::optional<basic_value<string_t>> parser<string_t, parsing_t, accel_traits>::parse(const parsing_t& content)
{
    return parser<string_t, parsing_t, accel_traits>(content.cbegin(), content.cend()).parse();
}

template <typename string_t, typename parsing_t, typename accel_traits>
inline std::optional<basic_value<string_t>> parser<string_t, parsing_t, accel_traits>::parse()
{
    if (!skip_whitespace()) {
        return std::nullopt;
    }

    basic_value<string_t> result_value;
    switch (*_cur) {
    case '[':
        result_value = parse_array();
        break;
    case '{':
        result_value = parse_object();
        break;
    default: // A JSON payload should be an basic_object or basic_array
        return std::nullopt;
    }

    if (!result_value.valid()) {
        return std::nullopt;
    }

    // After the parsing is complete, there should be no more content other than
    // spaces behind
    if (skip_whitespace()) {
        return std::nullopt;
    }

    return result_value;
}

template <typename string_t, typename parsing_t, typename accel_traits>
inline basic_value<string_t> parser<string_t, parsing_t, accel_traits>::parse_value()
{
    switch (*_cur) {
    case 'n':
        return parse_null();
    case 't':
    case 'f':
        return parse_boolean();
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return parse_number();
    case '"':
        return parse_string();
    case '[':
        return parse_array();
    case '{':
        return parse_object();
    default:
        return invalid_value<string_t>();
    }
}

template <typename string_t, typename parsing_t, typename accel_traits>
inline basic_value<string_t> parser<string_t, parsing_t, accel_traits>::parse_null()
{
    for (const auto& ch : _utils::null_string<string_t>()) {
        if (_cur != _end && *_cur == ch) {
            ++_cur;
        }
        else {
            return invalid_value<string_t>();
        }
    }

    return basic_value<string_t>();
}

template <typename string_t, typename parsing_t, typename accel_traits>
inline basic_value<string_t> parser<string_t, parsing_t, accel_traits>::parse_boolean()
{
    switch (*_cur) {
    case 't':
        for (const auto& ch : _utils::true_string<string_t>()) {
            if (_cur != _end && *_cur == ch) {
                ++_cur;
            }
            else {
                return invalid_value<string_t>();
            }
        }
        return true;
    case 'f':
        for (const auto& ch : _utils::false_string<string_t>()) {
            if (_cur != _end && *_cur == ch) {
                ++_cur;
            }
            else {
                return invalid_value<string_t>();
            }
        }
        return false;
    default:
        return invalid_value<string_t>();
    }
}

template <typename string_t, typename parsing_t, typename accel_traits>
inline basic_value<string_t> parser<string_t, parsing_t, accel_traits>::parse_number()
{
    const auto first = _cur;
    if (*_cur == '-') {
        ++_cur;
    }

    // numbers cannot have leading zeroes
    if (_cur != _end && *_cur == '0' && _cur + 1 != _end && std::isdigit(*(_cur + 1))) {
        return invalid_value<string_t>();
    }

    if (!skip_digit()) {
        return invalid_value<string_t>();
    }

    if (*_cur == '.') {
        ++_cur;
        if (!skip_digit()) {
            return invalid_value<string_t>();
        }
    }

    if (*_cur == 'e' || *_cur == 'E') {
        if (++_cur == _end) {
            return invalid_value<string_t>();
        }
        if (*_cur == '+' || *_cur == '-') {
            ++_cur;
        }
        if (!skip_digit()) {
            return invalid_value<string_t>();
        }
    }

    return basic_value<string_t>(basic_value<string_t>::value_type::number, string_t(first, _cur));
}

template <typename string_t, typename parsing_t, typename accel_traits>
inline basic_value<string_t> parser<string_t, parsing_t, accel_traits>::parse_string()
{
    auto string_opt = parse_stdstring();
    if (!string_opt) {
        return invalid_value<string_t>();
    }
    return basic_value<string_t>(basic_value<string_t>::value_type::string, std::move(string_opt).value());
}

template <typename string_t, typename parsing_t, typename accel_traits>
inline basic_value<string_t> parser<string_t, parsing_t, accel_traits>::parse_array()
{
    if (*_cur == '[') {
        ++_cur;
    }
    else {
        return invalid_value<string_t>();
    }

    if (!skip_whitespace()) {
        return invalid_value<string_t>();
    }
    else if (*_cur == ']') {
        ++_cur;
        // empty basic_array
        return basic_array<string_t>();
    }

    typename basic_array<string_t>::raw_array result;
    while (true) {
        if (!skip_whitespace()) {
            return invalid_value<string_t>();
        }

        basic_value<string_t> val = parse_value();

        if (!val.valid() || !skip_whitespace()) {
            return invalid_value<string_t>();
        }

        result.emplace_back(std::move(val));

        if (*_cur == ',') {
            ++_cur;
        }
        else {
            break;
        }
    }

    if (skip_whitespace() && *_cur == ']') {
        ++_cur;
    }
    else {
        return invalid_value<string_t>();
    }

    return basic_array<string_t>(std::move(result));
}

template <typename string_t, typename parsing_t, typename accel_traits>
inline basic_value<string_t> parser<string_t, parsing_t, accel_traits>::parse_object()
{
    if (*_cur == '{') {
        ++_cur;
    }
    else {
        return invalid_value<string_t>();
    }

    if (!skip_whitespace()) {
        return invalid_value<string_t>();
    }
    else if (*_cur == '}') {
        ++_cur;
        // empty basic_object
        return basic_object<string_t>();
    }

    typename basic_object<string_t>::raw_object result;
    while (true) {
        if (!skip_whitespace()) {
            return invalid_value<string_t>();
        }

        auto key_opt = parse_stdstring();

        if (key_opt && skip_whitespace() && *_cur == ':') {
            ++_cur;
        }
        else {
            return invalid_value<string_t>();
        }

        if (!skip_whitespace()) {
            return invalid_value<string_t>();
        }

        basic_value<string_t> val = parse_value();

        if (!val.valid() || !skip_whitespace()) {
            return invalid_value<string_t>();
        }

        result.emplace(std::move(*key_opt), std::move(val));

        if (*_cur == ',') {
            ++_cur;
        }
        else {
            break;
        }
    }

    if (skip_whitespace() && *_cur == '}') {
        ++_cur;
    }
    else {
        return invalid_value<string_t>();
    }

    return basic_object<string_t>(std::move(result));
}

template <typename string_t, typename parsing_t, typename accel_traits>
inline std::optional<string_t> parser<string_t, parsing_t, accel_traits>::parse_stdstring()
{
    if (*_cur == '"') {
        ++_cur;
    }
    else {
        return std::nullopt;
    }

    string_t result;
    auto no_escape_beg = _cur;

    while (_cur != _end) {
        if constexpr (sizeof(*_cur) == 1 && accel_traits::available) {
            if (!skip_string_literal_with_accel()) {
                return std::nullopt;
            }
        }
        switch (*_cur) {
        case '\t':
        case '\r':
        case '\n':
            return std::nullopt;
        case '\\': {
            result += string_t(no_escape_beg, _cur++);
            if (_cur == _end) {
                return std::nullopt;
            }
            switch (*_cur) {
            case '"':
                result.push_back('"');
                break;
            case '\\':
                result.push_back('\\');
                break;
            case '/':
                result.push_back('/');
                break;
            case 'b':
                result.push_back('\b');
                break;
            case 'f':
                result.push_back('\f');
                break;
            case 'n':
                result.push_back('\n');
                break;
            case 'r':
                result.push_back('\r');
                break;
            case 't':
                result.push_back('\t');
                break;
                // case 'u':
                //     result.push_back('\u');
                //     break;
            default:
                // Illegal backslash escape
                return std::nullopt;
            }
            no_escape_beg = ++_cur;
            break;
        }
        case '"': {
            result += string_t(no_escape_beg, _cur++);
            return result;
        }
        default:
            ++_cur;
            break;
        }
    }
    return std::nullopt;
}

template <typename string_t, typename parsing_t, typename accel_traits>
inline bool parser<string_t, parsing_t, accel_traits>::skip_string_literal_with_accel()
{
    if constexpr (sizeof(*_cur) != 1) {
        return false;
    }

    while (_end - _cur >= accel_traits::step) {
        auto pack = accel_traits::load_unaligned(&(*_cur));
        auto result = accel_traits::less(pack, 32);
        result = accel_traits::bitwise_or(result, accel_traits::equal(pack, static_cast<uint8_t>('"')));
        result = accel_traits::bitwise_or(result, accel_traits::equal(pack, static_cast<uint8_t>('\\')));

        if (accel_traits::is_all_zero(result)) {
            _cur += accel_traits::step;
        }
        else {
            auto index = accel_traits::first_nonzero_byte(result);
            _cur += index;
            break;
        }
    }

    return _cur != _end;
}

template <typename string_t, typename parsing_t, typename accel_traits>
inline bool parser<string_t, parsing_t, accel_traits>::skip_whitespace() noexcept
{
    while (_cur != _end) {
        switch (*_cur) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            ++_cur;
            break;
        case '\0':
            return false;
        default:
            return true;
        }
    }
    return false;
}

template <typename string_t, typename parsing_t, typename accel_traits>
inline bool parser<string_t, parsing_t, accel_traits>::skip_digit()
{
    // At least one digit
    if (_cur != _end && std::isdigit(*_cur)) {
        ++_cur;
    }
    else {
        return false;
    }

    while (_cur != _end && std::isdigit(*_cur)) {
        ++_cur;
    }

    if (_cur != _end) {
        return true;
    }
    else {
        return false;
    }
}

// *************************
// *      utils impl       *
// *************************

template <typename parsing_t>
auto parse(const parsing_t& content)
{
    using string_t = std::basic_string<typename parsing_t::value_type>;
    return parser<string_t, parsing_t>::parse(content);
}

template <typename char_t>
auto parse(char_t* content)
{
    return parse(std::basic_string_view<std::decay_t<char_t>> { content });
}

template <typename istream_t, typename _>
auto parse(istream_t& ifs, bool check_bom)
{
    using string_t = std::basic_string<typename istream_t::char_type>;

    ifs.seekg(0, std::ios::end);
    auto file_size = ifs.tellg();

    ifs.seekg(0, std::ios::beg);
    string_t str(file_size, '\0');

    ifs.read(str.data(), file_size);

    if (check_bom) {
        using uchar = unsigned char;
        static constexpr uchar Bom_0 = 0xEF;
        static constexpr uchar Bom_1 = 0xBB;
        static constexpr uchar Bom_2 = 0xBF;

        if (str.size() >= 3 && static_cast<uchar>(str.at(0)) == Bom_0 && static_cast<uchar>(str.at(1)) == Bom_1 &&
            static_cast<uchar>(str.at(2)) == Bom_2) {
            str.assign(str.begin() + 3, str.end());
        }
    }
    return parse(str);
}

template <typename ifstream_t, typename path_t>
auto open(const path_t& filepath, bool check_bom)
{
    using char_t = typename ifstream_t::char_type;
    using string_t = std::basic_string<char_t>;
    using json_t = json::basic_value<string_t>;
    using return_t = std::optional<json_t>;

    ifstream_t ifs(filepath, std::ios::in);
    if (!ifs.is_open()) {
        return return_t(std::nullopt);
    }
    auto opt = parse(ifs, check_bom);
    ifs.close();
    return opt;
}

namespace literals
{
    inline value operator""_json(const char* str, size_t len)
    {
        return operator""_jvalue(str, len);
    }
    inline wvalue operator""_json(const wchar_t* str, size_t len)
    {
        return operator""_jvalue(str, len);
    }

    inline value operator""_jvalue(const char* str, size_t len)
    {
        return parse(std::string_view(str, len)).value_or(value());
    }
    inline wvalue operator""_jvalue(const wchar_t* str, size_t len)
    {
        return parse(std::wstring_view(str, len)).value_or(wvalue());
    }

    inline array operator""_jarray(const char* str, size_t len)
    {
        auto val = parse(std::string_view(str, len)).value_or(value());
        return val.is_array() ? val.as_array() : array();
    }
    inline warray operator""_jarray(const wchar_t* str, size_t len)
    {
        auto val = parse(std::wstring_view(str, len)).value_or(wvalue());
        return val.is_array() ? val.as_array() : warray();
    }

    inline object operator""_jobject(const char* str, size_t len)
    {
        auto val = parse(std::string_view(str, len)).value_or(value());
        return val.is_object() ? val.as_object() : object();
    }
    inline wobject operator""_jobject(const wchar_t* str, size_t len)
    {
        auto val = parse(std::wstring_view(str, len)).value_or(wvalue());
        return val.is_object() ? val.as_object() : wobject();
    }
} // namespace literals

template <typename string_t>
const basic_value<string_t> invalid_value()
{
    return basic_value<string_t>(basic_value<string_t>::value_type::invalid, typename basic_value<string_t>::var_t());
}
} // namespace json
