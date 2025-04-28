// IWYU pragma: private, include <meojson/json.hpp>

#pragma once

#include <exception>
#include <string>

namespace json
{
class exception : public std::exception
{
public:
    exception() = default;

    exception(const std::string& msg)
        : _what(msg)
    {
    }

    exception(const exception&) = default;
    exception& operator=(const exception&) = default;
    exception(exception&&) = default;
    exception& operator=(exception&&) = default;

    virtual ~exception() noexcept override = default;

    virtual const char* what() const noexcept override
    {
        return _what.empty() ? "Unknown exception" : _what.c_str();
    }

protected:
    std::string _what;
};
}
