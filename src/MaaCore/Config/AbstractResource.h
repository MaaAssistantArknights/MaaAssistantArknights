#pragma once

#include <filesystem>
#include <meojson/json.hpp>

#include "Utils/SingletonHolder.hpp"

namespace asst
{
class AbstractResource
{
public:
    virtual ~AbstractResource() = default;
    virtual bool load(const std::filesystem::path& path) = 0;

public:
    AbstractResource(const AbstractResource& rhs) = delete;
    AbstractResource(AbstractResource&& rhs) noexcept = delete;

    AbstractResource& operator=(const AbstractResource& rhs) = delete;
    AbstractResource& operator=(AbstractResource&& rhs) noexcept = delete;

protected:
    AbstractResource() = default;
};
}
