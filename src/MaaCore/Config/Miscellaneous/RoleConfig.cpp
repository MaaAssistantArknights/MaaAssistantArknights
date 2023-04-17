#include "RoleConfig.h"

#include "Utils/Ranges.hpp"
#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::RoleConfig::parse(const json::value& json)
{
    LogTraceFunction;

    clear();
    const auto& roleFile = json.as_object();
    tagname = roleFile.at("flag").as_string();
    for (const auto& name : roleFile.at("roles").as_array()) {
        std::string rolename = name.as_string();
        role_all_name.insert(rolename);
    }
    return true;
}

void asst::RoleConfig::clear()
{
    role_all_name.clear();
    tagname = "";
}
