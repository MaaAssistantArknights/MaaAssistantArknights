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
    const int modeInt = params.get("mode", static_cast<int>(ReclamationMode::ProsperityInSave));
    const auto mode = static_cast<ReclamationMode>(modeInt);
    if (!is_valid_mode(mode, theme)) {
        Log.error(__FUNCTION__, "| Reclamation Algorithm mode", modeInt, "is incompatible with theme", theme);
        return false;
    }
    m_mode = mode;

    // ———————— ReclamationMode::ProsperityInSave 专用参数 ————————————————————————————
    if (m_mode == ReclamationMode::ProsperityInSave) {
        m_tool_to_craft = params.get("tool_to_craft", "荧光棒");
    }

    return true;
}
} // namespace asst
