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
        // 重启锚点: RA-1 = 1 << 4, RA-15 = 2 << 4
        if (modeInt < 0 || modeInt >= static_cast<int>(RelaunchAnchorMode::_Count)) {
            Log.error(__FUNCTION__, "| Invalid RelaunchAnchor mode", modeInt);
            return false;
        }
        m_mode = static_cast<RelaunchAnchorMode>(modeInt);
    }
    else {
        // 沙洲遗闻：mode 0 = 无存档刷繁荣点数, mode 1 = 有存档刷繁荣点数
        if (modeInt < 0 || modeInt >= static_cast<int>(TalesMode::_Count)) {
            Log.error(__FUNCTION__, "| Invalid Tales mode", modeInt);
            return false;
        }
        m_mode = static_cast<TalesMode>(modeInt);
    }

    return true;
}
