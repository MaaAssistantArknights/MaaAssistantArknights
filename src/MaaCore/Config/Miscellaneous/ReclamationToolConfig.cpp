#include "ReclamationToolConfig.h"

#include <algorithm>

#include "Utils/Logger.hpp"

bool asst::ReclamationToolConfig::parse(const json::value& json)
{
    if (!json.is<Tools>() || json.empty()) {
        LogError << "Invalid Reclamation tool catalog";
        return false;
    }
    auto tools = json.as<Tools>();
    for (const auto& [id, names] : tools) {
        if (id.empty() || names.empty() || std::ranges::find(names, "") != names.end()) {
            LogError << "Missing Reclamation tool name for" << id;
            return false;
        }
    }
    m_tools = std::move(tools);
    return true;
}

std::vector<std::string> asst::ReclamationToolConfig::names_for(const std::string& tool) const
{
    if (auto it = m_tools.find(tool); it != m_tools.end()) {
        return it->second;
    }
    const std::vector<std::string>* match = nullptr;
    for (const auto& [id, names] : m_tools) {
        if (std::ranges::find(names, tool) == names.end()) {
            continue;
        }
        if (match) {
            // A shared display name cannot identify which recipe the caller intended.
            return { tool };
        }
        match = &names;
    }
    return match ? *match : std::vector<std::string> { tool };
}
