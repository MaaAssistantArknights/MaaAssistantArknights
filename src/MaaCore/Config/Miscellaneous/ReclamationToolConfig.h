#pragma once

#include "Config/AbstractConfig.h"

#include <map>
#include <vector>

namespace asst
{
class ReclamationToolConfig final : public MAA_NS::SingletonHolder<ReclamationToolConfig>, public AbstractConfig
{
public:
    std::vector<std::string> names_for(const std::string& tool) const;

protected:
    bool parse(const json::value& json) override;

private:
    using Tools = std::map<std::string, std::vector<std::string>>;
    Tools m_tools;
};
}
