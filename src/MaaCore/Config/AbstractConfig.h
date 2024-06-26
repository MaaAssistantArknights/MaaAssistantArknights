#pragma once

#include "AbstractResource.h"

#include <future>
#include <mutex>

#include <meojson/json.hpp>

namespace asst
{
class AbstractConfig : public AbstractResource
{
public:
    virtual ~AbstractConfig() override = default;
    virtual bool load(const std::filesystem::path& path) override;

protected:
    virtual bool parse(const json::value& json) = 0;

    std::filesystem::path m_path;
};
}
