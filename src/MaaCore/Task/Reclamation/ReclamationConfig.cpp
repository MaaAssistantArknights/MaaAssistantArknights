#include "ReclamationConfig.h"

#include "Utils/Logger.hpp"

bool asst::ReclamationConfig::verify_and_load_params(const json::value& params)
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
    if (theme == ReclamationTheme::RelaunchAnchor) {
        m_mode = ReclamationMode::ProsperityNoSave;
        if (!params.contains("stage")) {
            Log.error(__FUNCTION__, "| Missing RelaunchAnchor stage");
            return false;
        }

        const int stageInt = params.get("stage", -1);
        if (stageInt == static_cast<int>(RelaunchAnchorStage::RA15)) {
            m_stage = RelaunchAnchorStage::RA15;
        } else if (stageInt == static_cast<int>(RelaunchAnchorStage::RA1)) {
            m_stage = RelaunchAnchorStage::RA1;
        } else {
            Log.error(__FUNCTION__, "| Invalid RelaunchAnchor stage", stageInt);
            return false;
        }
        return true;
    }

    if (!is_valid_mode(mode, theme)) {
        Log.error(__FUNCTION__, "| Reclamation Algorithm mode", modeInt, "is incompatible with theme", theme);
        return false;
    }
    m_mode = mode;

    return true;
}
