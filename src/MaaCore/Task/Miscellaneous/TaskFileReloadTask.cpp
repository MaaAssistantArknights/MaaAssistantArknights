#include "TaskFileReloadTask.h"

#include "Config/Miscellaneous/CopilotConfig.h"
#include "Utils/Logger.hpp"

void asst::TaskFileReloadTask::set_magic_code(std::string code)
{
    is_magic_code = true;
    m_file = std::move(code);
}

void asst::TaskFileReloadTask::set_path(std::string path)
{
    is_path = true;
    m_file = std::move(path);
}

bool asst::TaskFileReloadTask::_run()
{
    if (is_magic_code) {
        if (!Copilot.parse_magic_code(m_file)) {
            Log.error("CopilotConfig parse failed");
            return false;
        }
    }
    else if (is_path) {
        if (!Copilot.load(utils::path(m_file))) {
            Log.error("CopilotConfig parse failed");
            return false;
        }
    }
    else {
        return false;
    }

    json::value info = basic_info_with_what("CopilotListLoadTaskFileSuccess");
    info["details"]["stage_name"] = Copilot.get_stage_name();
    info["details"]["file_name"] = std::move(m_file);
    callback(AsstMsg::SubTaskExtraInfo, info);
    return true;
}
