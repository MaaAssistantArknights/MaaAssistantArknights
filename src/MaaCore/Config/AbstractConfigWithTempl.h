#pragma once

#include "AbstractConfig.h"

#include <unordered_set>

namespace asst
{
class AbstractConfigWithTempl : public AbstractConfig
{
public:
    virtual ~AbstractConfigWithTempl() override = default;

    virtual const std::unordered_set<std::string>& get_templ_required() const noexcept = 0;
};
}
