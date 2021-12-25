#pragma once

#include "AbstractResource.h"

#include <string>

namespace json
{
    class value;
}

namespace asst
{
    class AbstractConfiger : public AbstractResource
    {
    public:
        virtual ~AbstractConfiger() = default;
        virtual bool load(const std::string& filename) override;

    protected:
        virtual bool parse(const json::value& json) = 0;
    };
}
