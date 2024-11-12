#pragma once

#include "Config/AbstractConfig.h"

#include "Utils/Ranges.hpp"
#include <algorithm>
#include <numeric>
#include <string>
#include <unordered_set>
#include <vector>

#include "Common/AsstTypes.h"

namespace asst
{
class OcrConfig final : public SingletonHolder<OcrConfig>, public AbstractConfig
{
public:
    virtual ~OcrConfig() override = default;

    std::string process_equivalence_class(const std::string& str) const;

    auto get_eq_classes() const noexcept { return m_eq_classes; }

protected:
    virtual bool parse(const json::value& json) override;

    using equivalence_class = std::vector<std::string>;

    std::vector<equivalence_class> m_eq_classes;
};
} // namespace asst
