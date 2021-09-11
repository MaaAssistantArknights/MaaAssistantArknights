#pragma once

#include <string>
#include <optional>

namespace json
{
    class value;
    class object;
    class array;
    enum class value_type : char;

    class parser
    {
    public:
        ~parser() noexcept = default;

        static std::optional<value> parse(const std::string& content);

    private:
        parser(
            const std::string::const_iterator& cbegin,
            const std::string::const_iterator& cend) noexcept
            : _cur(cbegin), _end(cend) {}

        std::optional<value> parse();
        value parse_value();

        value parse_null();
        value parse_boolean();
        value parse_number();
        // parse and return a json::value whose type is value_type::String
        value parse_string();
        value parse_array();
        value parse_object();

        // parse and return a std::string
        std::optional<std::string> parse_stdstring();

        bool skip_whitespace() noexcept;
        bool skip_digit() noexcept;

    private:
        std::string::const_iterator _cur;
        std::string::const_iterator _end;
    };

    std::optional<value> parse(const std::string& content);

} // namespace json