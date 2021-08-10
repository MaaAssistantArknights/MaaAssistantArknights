#pragma once

#include <exception>
#include <string>

namespace json
{
    class exception : public std::exception
    {
    public:
        exception() = default;
        exception(const std::string& msg);

        exception(const exception&) = default;
        exception& operator=(const exception&) = default;
        exception(exception&&) = default;
        exception& operator=(exception&&) = default;

        virtual ~exception() noexcept override = default;

        virtual const char* what() const noexcept override;

    private:
        std::string m_msg;
    };

} // namespace json