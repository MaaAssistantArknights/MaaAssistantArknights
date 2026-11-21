#include "ReclamationConfig.h"

#include "Utils/Logger.hpp"

std::shared_ptr<const asst::ReclamationConfig::Params> asst::ReclamationConfig::get_params() const
{
    std::lock_guard lock(m_mutex);
    return m_params;
}

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
    auto next = std::make_shared<Params>();
    next->theme = theme;

    // Reclamation Algorithm Mode
    const int modeInt = params.get("mode", 0);
    if (theme == ReclamationTheme::RelaunchAnchor) {
        // 重启锚点：mode 使用 flags 风格的显式值编码，RA-1 = 1 << 4，RA-15 = 2 << 4
        switch (modeInt) {
        case static_cast<int>(RelaunchAnchorMode::RA1):
            next->mode = RelaunchAnchorMode::RA1;
            break;

        case static_cast<int>(RelaunchAnchorMode::RA15):
            next->mode = RelaunchAnchorMode::RA15;
            break;

        case static_cast<int>(RelaunchAnchorMode::RA4):
            next->mode = RelaunchAnchorMode::RA4;
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
            next->mode = TalesMode::ProsperityNoSave;
            next->clear_store = params.get("clear_store", false);
            break;

        case static_cast<int>(TalesMode::ProsperityInSave): {
            next->mode = TalesMode::ProsperityInSave;
            const auto increment =
                static_cast<IncrementMode>(params.get("increment_mode", static_cast<int>(IncrementMode::Click)));
            if (increment != IncrementMode::Click && increment != IncrementMode::Hold) {
                LogError << __FUNCTION__ << "unknown increment mode" << static_cast<int>(increment);
                break;
            }
            next->increment_mode = increment;
            next->num_craft_batches = params.get("num_craft_batches", next->num_craft_batches);
            for (const auto& tool : params.get("tools_to_craft", json::array {})) {
                if (auto name = tool.as_string(); !name.empty()) {
                    next->tools_to_craft.emplace_back(std::move(name));
                }
            }
            break;
        }

        default:
            Log.error(__FUNCTION__, "| Invalid Tales mode", modeInt);
            return false;
        }
    }

    std::lock_guard lock(m_mutex);
    m_params = std::move(next);
    return true;
}
