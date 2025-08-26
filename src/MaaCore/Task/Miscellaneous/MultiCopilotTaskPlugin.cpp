#include "MultiCopilotTaskPlugin.h"

#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/TaskData.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

bool asst::MultiCopilotTaskPlugin::_run()
{
    if (m_copilot_configs.size() < (size_t)m_index_current) {
        LogError << __FUNCTION__ << "configs size:" << m_copilot_configs.size() << ", current index:" << m_index_current
                 << ", out of range";
        return false;
    }

    const auto& config = m_copilot_configs[m_index_current];

    std::string file_name;
    if (std::holds_alternative<int>(config.copilot_file)) {
        if (!Copilot.parse_magic_code(std::to_string(std::get<int>(config.copilot_file)))) {
            Log.error("CopilotConfig parse failed");
            return false;
        }
        file_name = std::to_string(std::get<int>(config.copilot_file));
    }
    else if (std::holds_alternative<std::filesystem::path>(config.copilot_file)) {
        if (!Copilot.load(std::get<std::filesystem::path>(config.copilot_file))) {
            Log.error("CopilotConfig parse failed");
            return false;
        }
        file_name = utils::path_to_utf8_string(std::get<std::filesystem::path>(config.copilot_file));
    }
    else {
        return false;
    }

    json::value info = basic_info_with_what("CopilotListLoadTaskFileSuccess");
    info["details"]["stage_name"] = Copilot.get_stage_name();
    info["details"]["file_name"] = std::move(file_name);
    callback(AsstMsg::SubTaskExtraInfo, info);

    if (config.is_paradox) {
        // 悖论模拟走自己的导航逻辑
        m_index_current++;
        return true;
    }

    bool ret = true;
    Task.get<OcrTaskInfo>(config.nav_name + "@Copilot@ClickStageName")->text = { config.nav_name };
    std::string replace_navigate_name = config.nav_name;
    utils::string_replace_all_in_place(replace_navigate_name, { { "-", "" } });
    Task.get<OcrTaskInfo>(config.nav_name + "@Copilot@ClickedCorrectStage")->text = { config.nav_name,
                                                                                      replace_navigate_name };
    ret = ret && ProcessTask(*this, { config.nav_name + "@Copilot@StageNavigationBegin" }).run();

    ProcessTask(*this, { "NotUsePrts" }).set_ignore_error(true).set_retry_times(0).run();
    if (config.is_raid) {
        // 选择突袭模式
        ret = ret && ProcessTask(*this, { "RaidConfirm", "ChangeToRaidDifficulty" }).run();
    }

    m_index_current++;
    return ret;
}
