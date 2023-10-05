#include "RoguelikeCiphertextBoardGainTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

bool asst::RoguelikeCiphertextBoardGainTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (m_roguelike_theme.empty()) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    // 暂时只有萨米有密文板，下个肉鸽没有密文板的话考虑在roguelike中建个子文件夹
    if (task_view == "Sami@Roguelike@CiphertextBoardGain") {
        m_ocr_after_combat = false;
        return true;
    }
    if (task_view == "Sami@Roguelike@CiphertextBoardGain") {
        m_ocr_after_combat = false;
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeCiphertextBoardGainTaskPlugin::_run()
{
    LogTraceFunction;

    std::string theme = m_roguelike_theme;
    std::string mode = status()->get_properties(Status::RoguelikeMode).value();

    std::string task_name = theme + "@Roguelike@CiphertextBoardGainOcr";
    OCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info(task_name);
    if (!analyzer.analyze()) {
        return false;
    }

    std::string ciphertext_board = analyzer.get_result().front().text;    
    std::string overview_str =
        status()->get_str(Status::RoguelikeCiphertextBoardOverview).value_or(json::value().to_string());

    auto& overview = json::parse(overview_str).value_or(json::value()).as_array();
    // 把ciphertext_board存到overview里
    overview.push_back(ciphertext_board);
    status()->set_str(Status::RoguelikeCiphertextBoardOverview, overview.to_string());

    return true;
}
