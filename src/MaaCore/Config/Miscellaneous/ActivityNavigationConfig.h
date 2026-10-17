#pragma once

#include <optional>
#include <shared_mutex>
#include <unordered_map>

#include "ActivityNavigationPolicy.h"
#include "Config/AbstractConfig.h"

namespace asst
{
class ActivityNavigationConfig final : public MAA_NS::SingletonHolder<ActivityNavigationConfig>, public AbstractConfig
{
public:
    std::optional<activity_navigation::Route> find(const std::string& stage_id) const;
    std::optional<activity_navigation::Route> find_code(const std::string& code) const;

protected:
    bool parse(const json::value& json) override;

private:
    mutable std::shared_mutex m_routes_mutex;
    std::unordered_map<std::string, activity_navigation::Route> m_routes;
};

inline auto& ActivityNavigation = ActivityNavigationConfig::get_instance();
}
