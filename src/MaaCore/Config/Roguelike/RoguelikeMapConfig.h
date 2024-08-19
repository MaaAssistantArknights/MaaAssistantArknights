#pragma once

#include "Config/AbstractConfig.h"
#include "Task/Roguelike/Map/RoguelikeMap.h"

namespace asst
{
class RoguelikeMapConfig final : public SingletonHolder<RoguelikeMapConfig>, public AbstractConfig
{
public:
    virtual ~RoguelikeMapConfig() override = default;

    const RoguelikeNodeType& templ2type(const std::string& theme, const std::string& templ_name) const noexcept
    {
        const auto& templ_type_mapping = m_templ_type_mappings.at(theme);
        return templ_type_mapping.at(templ_name);
    }

private:
    virtual bool parse(const json::value& json) override;

    void clear();

    std::unordered_map<std::string, std::unordered_map<std::string, RoguelikeNodeType>> m_templ_type_mappings;
};

inline static auto& RoguelikeMapInfo = RoguelikeMapConfig::get_instance();
}
