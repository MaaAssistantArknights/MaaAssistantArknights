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
    const int modeInt = params.get("mode", 0);
    if (theme == ReclamationTheme::RelaunchAnchor) {
        // 重启锚点：mode 使用 flags 风格的显式值编码，RA-1 = 1 << 4，RA-15 = 2 << 4
        switch (modeInt) {
        case static_cast<int>(RelaunchAnchorMode::RA1):
            m_mode = RelaunchAnchorMode::RA1;
            break;

        case static_cast<int>(RelaunchAnchorMode::RA15):
            m_mode = RelaunchAnchorMode::RA15;
            break;

        case static_cast<int>(RelaunchAnchorMode::CollectEnergy):
            m_mode = RelaunchAnchorMode::CollectEnergy;
            break;

        default:
            Log.error(__FUNCTION__, "| Invalid RelaunchAnchor mode", modeInt);
            return false;
        }
    }
    else {
        // 沙洲遗闻：mode 使用连续值编码，0 = 无存档刷繁荣点数，1 = 有存档刷繁荣点数
        switch (modeInt) {
        case static_cast<int>(TalesMode::ProsperityNoSave):
            m_mode = TalesMode::ProsperityNoSave;
            break;

        case static_cast<int>(TalesMode::ProsperityInSave):
            m_mode = TalesMode::ProsperityInSave;
            break;

        default:
            Log.error(__FUNCTION__, "| Invalid Tales mode", modeInt);
            return false;
        }
    }

    return true;
}
