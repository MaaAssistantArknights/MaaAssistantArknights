#include "RoguelikeCiphertextBoardGainTaskPlugin.h"

#include "Controller/Controller.h"
#include "Status.h"
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
    if (m_roguelike_theme != "Sami") {
        return false;
    }
    const std::string roguelike_name = m_roguelike_theme + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;

    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@CiphertextBoardGain") {
        m_ocr_after_combat = false;
        return true;
    }
    if (task_view == "Roguelike@StageEncounterSpecialClose") {
        m_ocr_after_combat = false;
        return true;
    }
    if (task_view == "Roguelike@GetDropSelectReward2") {
        m_ocr_after_combat = true;
        return true;
    }
    /* 可能是没点科技树会单独掉一个密文板，很快能点出来，暂时不做识别
    if (task_view == "Roguelike@GetDrop2") {
        m_ocr_after_combat = true;
        return true;
    }*/
    else {
        return false;
    }
}

bool asst::RoguelikeCiphertextBoardGainTaskPlugin::_run()
{
    LogTraceFunction;

    std::string theme = m_roguelike_theme;
    std::string mode = status()->get_properties(Status::RoguelikeMode).value();

    auto image = ctrler()->get_image();

    OCRer analyzer(image);
    std::string task_name = m_ocr_after_combat ? theme + "@Roguelike@CiphertextBoardGainOcrAfterCombat"
                                               : theme + "@Roguelike@CiphertextBoardGainOcr";
    analyzer.set_task_info(task_name);

    if (!analyzer.analyze()) {
        return false;
    }
    std::string ciphertext_board = analyzer.get_result().front().text;

    std::string overview_str =
        status()->get_str(Status::RoguelikeCiphertextBoardOverview).value_or(json::value().to_string());
    json::value overview_json = json::parse(overview_str).value_or(json::value());
    auto& overview = overview_json.as_array();
    // 把ciphertext_board存到overview里
    overview.emplace_back(ciphertext_board);
    status()->set_str(Status::RoguelikeCiphertextBoardOverview, overview.to_string());

    if (m_ocr_after_combat) {
        ctrler()->click(analyzer.get_result().front().rect);
    }

    return true;
}
