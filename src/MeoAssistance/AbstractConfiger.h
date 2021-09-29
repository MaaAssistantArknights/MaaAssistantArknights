#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "AsstDef.h"

namespace json {
    class value;
}

namespace asst {
    class AbstractConfiger
    {
    public:
        virtual ~AbstractConfiger() = default;
        virtual bool load(const std::string& filename);

    protected:
        AbstractConfiger() = default;
        AbstractConfiger(const AbstractConfiger& rhs) = default;
        AbstractConfiger(AbstractConfiger&& rhs) noexcept = default;

        AbstractConfiger& operator=(const AbstractConfiger& rhs) = default;
        AbstractConfiger& operator=(AbstractConfiger&& rhs) noexcept = default;

        virtual bool parse(const json::value& json) = 0;
    };
}
