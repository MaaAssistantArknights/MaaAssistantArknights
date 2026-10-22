#pragma once

#include "Config/AbstractConfig.h"

#include <algorithm>
#include <numeric>
#include <ranges>
#include <string>
#include <unordered_set>
#include <vector>

#include "Common/AsstTypes.h"

namespace asst
{
class OcrConfig final : public MAA_NS::SingletonHolder<OcrConfig>, public AbstractConfig
{
public:
    virtual ~OcrConfig() override = default;

    std::string normalize_for_matching(const std::string& text) const;

protected:
    virtual bool parse(const json::value& json) override;

    using equivalence_class = std::vector<std::string>;

    std::vector<equivalence_class> m_eq_classes;
};
} // namespace asst
