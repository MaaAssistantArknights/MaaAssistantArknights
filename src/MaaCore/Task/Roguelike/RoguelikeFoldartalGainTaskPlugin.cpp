#include "RoguelikeFoldartalGainTaskPlugin.h"

#include "Controller/Controller.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

bool asst::RoguelikeFoldartalGainTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (m_config->get_theme().empty()) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    if (m_config->get_theme() != RoguelikeTheme::Sami) {
        return false;
    }
    const std::string roguelike_name = m_config->get_theme() + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;

    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    m_ocr_next_level = false;
    if (task_view == "Roguelike@FoldartalGain") {
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
    if (task_view == "Roguelike@NextLevel") {
        m_ocr_next_level = true;
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

bool asst::RoguelikeFoldartalGainTaskPlugin::_run()
{
    LogTraceFunction;

    if (m_ocr_next_level) {
        enter_next_floor();
        return true;
    }
    else {
        return after_combat();
    }
}

void asst::RoguelikeFoldartalGainTaskPlugin::enter_next_floor()
{
    auto foldartal_floor = m_config->get_foldartal_floor();

    // 到达第二层后，获得上一层预见的密文板
    if (foldartal_floor) {
        store_to_status(*foldartal_floor, Status::RoguelikeFoldartalOverview);
    }

    foldartal_floor.reset();
    OCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info(m_config->get_theme() + "@Roguelike@FoldartalGainOcrNextLevel");
    if (analyzer.analyze()) {
        foldartal_floor = analyzer.get_result().front().text;
    }
    m_config->set_foldartal_floor(std::move(foldartal_floor));
}

bool asst::RoguelikeFoldartalGainTaskPlugin::after_combat()
{
    OCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info(m_config->get_theme() + (m_ocr_after_combat ? "@Roguelike@FoldartalGainOcrAfterCombat"
                                                                       : "@Roguelike@FoldartalGainOcr"));
    if (!analyzer.analyze()) {
        return false;
    }

    auto foldartal = analyzer.get_result().front().text;
    store_to_status(std::move(foldartal), Status::RoguelikeFoldartalOverview);
    if (m_ocr_after_combat) {
        ctrler()->click(analyzer.get_result().front().rect);
    }

    return true;
}

bool asst::RoguelikeFoldartalGainTaskPlugin::store_to_status(std::string foldartal, auto& status_string)
{
    json::array overview = get_array(status_string);
    overview.emplace_back(foldartal);
    status()->set_str(status_string, overview.to_string());
    return true;
}

json::array asst::RoguelikeFoldartalGainTaskPlugin::get_array(auto& status_string)
{
    std::string overview_str = status()->get_str(status_string).value_or(json::value().to_string());
    json::value overview_json = json::parse(overview_str).value_or(json::value());
    return overview_json.as_array();
}
