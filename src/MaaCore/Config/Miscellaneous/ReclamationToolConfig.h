#pragma once

#include "Config/AbstractConfig.h"

#include <map>
#include <mutex>
#include <vector>

namespace asst
{
class ReclamationToolConfig final : public MAA_NS::SingletonHolder<ReclamationToolConfig>, public AbstractResource
{
public:
    bool load(const std::filesystem::path& path) override;
    bool load_layer(const std::filesystem::path& root, bool inherits);
    std::vector<std::string> names_for(const std::string& tool) const;

private:
    using Tools = std::map<std::string, std::vector<std::string>>;

    struct Catalog : AbstractConfig
    {
        bool parse(const json::value& json) override;
        Tools tools;
    };

    struct Layer
    {
        std::filesystem::path source;
        Tools tools;
    };

    void replace_layer(const std::filesystem::path& source, bool inherits, Tools tools);
    mutable std::mutex m_mutex;
    std::vector<Layer> m_layers;
};
}
