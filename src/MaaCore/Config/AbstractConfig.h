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

    // 直接从内存中的 json 解析（不走文件），供 ResourceLoader 在外部合并多份配置后调用
    bool load(const json::value& json);

protected:
    virtual bool parse(const json::value& json) = 0;

    std::filesystem::path m_path;
};
}
