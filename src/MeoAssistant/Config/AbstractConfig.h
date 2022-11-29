#pragma once

#include "AbstractResource.h"

namespace json
{
    class value;
}

namespace asst
{
    class AbstractConfig : public AbstractResource
    {
    public:
        virtual ~AbstractConfig() override = default;
        virtual bool load(const std::filesystem::path& path) override;

    protected:
        virtual bool parse(const json::value& json) = 0;
    };
}
