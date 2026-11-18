#include "ReclamationToolConfig.h"

#include <algorithm>

#include "Utils/Logger.hpp"

bool asst::ReclamationToolConfig::Catalog::parse(const json::value& json)
{
    if (!json.is<Tools>() || json.empty()) {
        LogError << "Invalid Reclamation tool catalog";
        return false;
    }
    auto names_by_id = json.as<Tools>();
    for (const auto& [id, names] : names_by_id) {
        if (id.empty() || names.empty() || std::ranges::find(names, "") != names.end()) {
            LogError << "Missing Reclamation tool name for" << id;
            return false;
        }
    }
    tools = std::move(names_by_id);
    return true;
}

bool asst::ReclamationToolConfig::load(const std::filesystem::path& path)
{
    Catalog catalog;
    if (!catalog.load(path)) {
        return false;
    }
    replace_layer(path, false, std::move(catalog.tools));
    return true;
}

bool asst::ReclamationToolConfig::load_layer(const std::filesystem::path& root, bool inherits)
{
    using namespace asst::utils::path_literals;

    const auto path = root / "reclamation/Tales/craft_names.json"_p;
    const auto custom = root / "craft_names_custom.json"_p;
    Catalog catalog;
    for (const auto& file : { path, custom }) {
        if (std::filesystem::exists(file) && !catalog.load(file)) {
            return false;
        }
    }
    replace_layer(path, inherits, std::move(catalog.tools));
    return true;
}

void asst::ReclamationToolConfig::replace_layer(const std::filesystem::path& source, bool inherits, Tools tools)
{
    const auto path = source.lexically_normal();
    std::lock_guard lock(m_mutex);
    if (inherits) {
        std::erase_if(m_layers, [&](const Layer& layer) { return layer.source == path; });
    }
    else {
        m_layers.clear();
    }
    if (!tools.empty()) {
        m_layers.emplace_back(path, std::move(tools));
    }
}

std::vector<std::string> asst::ReclamationToolConfig::names_for(const std::string& tool) const
{
    std::lock_guard lock(m_mutex);
    if (m_layers.empty()) {
        return { tool };
    }
    const auto& tools = m_layers.back().tools;
    if (auto it = tools.find(tool); it != tools.end()) {
        return it->second;
    }
    const std::vector<std::string>* match = nullptr;
    for (const auto& [id, names] : tools) {
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
