#include "ReclamationConfig.h"

#include "Utils/Logger.hpp"

namespace asst
{
bool ReclamationConfig::verify_and_load_params(const json::value& params)
{
    LogTraceFunction;

    // ———————— 通用参数 ——————————————————————————————————————————————————————————————————
    // Reclamation Algorithm Theme
    const std::string theme = params.get("theme", std::string(ReclamationTheme::Tales));
    if (!is_valid_theme(theme)) {
        Log.error(__FUNCTION__, "| Unknown Reclamation Algorithm theme", theme);
        return false;
    }
    m_theme = theme;

    // Reclamation Algorithm Mode
    const int strategyInt = params.get("strategy", static_cast<int>(ReclamationStrategy::ProsperityInSave));
    const auto strategy = static_cast<ReclamationStrategy>(strategyInt);
    if (!is_valid_strategy(strategy, theme)) {
        Log.error(__FUNCTION__, "| Reclamation Algorithm strategy", strategyInt, "is incompatible with theme", theme);
        return false;
    }
    m_strategy = strategy;

    // ———————— ReclamationStrategy::ProsperityInSave 专用参数 ————————————————————————————
    if (m_strategy == ReclamationStrategy::ProsperityInSave) {
        m_tool_to_craft = params.get("tool_to_craft", "荧光棒");
    }

    return true;
}

void ReclamationConfig::set_theme(const std::string& theme)
{
    if (!is_valid_theme(theme)) {
        Log.error(__FUNCTION__, "| Failed to set unknown Reclamation Algorithm theme", theme);
        return;
    }
    m_theme = theme;
}

void ReclamationConfig::set_strategy(const ReclamationStrategy& strategy)
{
    if (!is_valid_strategy(strategy, m_theme)) {
        Log.error(
            __FUNCTION__,
            "| Failed to set Reclamation Algorithm strategy",
            static_cast<int>(strategy),
            "since it is incompatible with current theme",
            m_theme);
        return;
    }
    m_strategy = strategy;
}

void ReclamationConfig::set_mode(const ReclamationMode& mode)
{
    if (!is_valid_mode(mode, m_strategy)) {
        Log.error(
            __FUNCTION__,
            "| Failed to set Reclamation Algorithm mode",
            static_cast<int>(mode),
            "since it is incompatible with current strategy",
            static_cast<int>(m_strategy));
        return;
    }
    m_mode = mode;
}
} // namespace asst
