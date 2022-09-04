#pragma once

#include "AbstractConfiger.h"

#include <unordered_set>

namespace asst
{
    class AbstractConfigerWithTempl : public AbstractConfiger
    {
    public:
        virtual ~AbstractConfigerWithTempl() override = default;

        virtual const std::unordered_set<std::string>& get_templ_required() const noexcept = 0;
    };
}
