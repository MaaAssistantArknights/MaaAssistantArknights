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

    std::string theme = m_config->get_theme();
    auto image = ctrler()->get_image();

    OCRer analyzer(image);
    std::string foldartal = "None";
    if (m_ocr_next_level) {
        analyzer.set_task_info(theme + "@Roguelike@FoldartalGainOcrNextLevel");
        if (analyzer.analyze()) {
            foldartal = analyzer.get_result().front().text;
        }
        store_to_status(foldartal, Status::RoguelikeFoldartalFloor);
        json::array foldartal_floor_array = get_array(Status::RoguelikeFoldartalFloor);
        // 到达第二层后，获得上一层预见的密文板
        if (foldartal_floor_array.size() >= 2) {
            std::string foldartal_last_floor = (foldartal_floor_array.end() - 2)->to_string();
            if (foldartal_last_floor.size() >= 2 && foldartal_last_floor.front() == '"' &&
                foldartal_last_floor.back() == '"') {
                foldartal_last_floor = foldartal_last_floor.substr(1, foldartal_last_floor.size() - 2);
            }
            if (foldartal_last_floor != "None") {
                store_to_status(foldartal_last_floor, Status::RoguelikeFoldartalOverview);
            }
        }
        return true;
<<<<<<< HEAD
    }
    else {
        return gain_stage_award();
    }
}
=======
    };
>>>>>>> parent of 94439e87c (refactor: 重构肉鸽每一层的预见密文板获取)

    std::string task_name =
        m_ocr_after_combat ? theme + "@Roguelike@FoldartalGainOcrAfterCombat" : theme + "@Roguelike@FoldartalGainOcr";
    analyzer.set_task_info(task_name);

<<<<<<< HEAD
    // 到达第二层后，获得上一层预见的密文板
    if (foldartal_floor) {
        gain_foldartal(*foldartal_floor);
    }

    foldartal_floor.reset();
    OCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info(m_config->get_theme() + "@Roguelike@FoldartalGainOcrNextLevel");
    if (analyzer.analyze()) {
        foldartal_floor = analyzer.get_result().front().text;
    }
    m_config->set_foldartal_floor(std::move(foldartal_floor));
}

bool asst::RoguelikeFoldartalGainTaskPlugin::gain_stage_award()
{
    OCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info(m_config->get_theme() + (m_ocr_after_combat ? "@Roguelike@FoldartalGainOcrAfterCombat"
                                                                       : "@Roguelike@FoldartalGainOcr"));
    if (!analyzer.analyze()) {
        return false;
    }

    auto foldartal = analyzer.get_result().front().text;
    gain_foldartal(std::move(foldartal));
=======
    if (!analyzer.analyze()) {
        return false;
    }
    foldartal = analyzer.get_result().front().text;
    store_to_status(foldartal, Status::RoguelikeFoldartalOverview);
>>>>>>> parent of 94439e87c (refactor: 重构肉鸽每一层的预见密文板获取)
    if (m_ocr_after_combat) {
        ctrler()->click(analyzer.get_result().front().rect);
    }

    return true;
}

void asst::RoguelikeFoldartalGainTaskPlugin::gain_foldartal(std::string name)
{
    auto foldartal_list = m_config->get_foldartal();
    foldartal_list.emplace_back(std::move(name));
    m_config->set_foldartal(std::move(foldartal_list));
}
