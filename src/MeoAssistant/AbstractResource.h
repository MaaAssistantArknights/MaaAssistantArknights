#pragma once

#include <string>

namespace asst
{
    class AbstractResource
    {
    public:
        AbstractResource() = default;
        AbstractResource(const AbstractResource& rhs) = delete;
        AbstractResource(AbstractResource&& rhs) noexcept = delete;

        virtual ~AbstractResource() = default;

        virtual bool load(const std::string& filename) = 0;
        virtual const std::string& get_last_error() const noexcept
        {
            return m_last_error;
        }

        AbstractResource& operator=(const AbstractResource& rhs) = delete;
        AbstractResource& operator=(AbstractResource&& rhs) noexcept = delete;

    protected:
        std::string m_last_error;
    };
}
