#pragma once

#include "AbstractResource.h"

#include <future>
#include <mutex>

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
        virtual void async_load(const std::filesystem::path& path);

    protected:
        virtual bool parse(const json::value& json) = 0;

        bool _load(std::filesystem::path path);

        std::filesystem::path m_path;
        std::future<bool> m_load_future;
        std::mutex m_load_mutex;
    };
}
