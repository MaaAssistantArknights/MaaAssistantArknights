#include "RoguelikeFoldartalGainTaskPlugin.h"

#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

bool asst::RoguelikeFoldartalGainTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (!RoguelikeConfig::is_valid_theme(m_config->get_theme())) {
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
        return gain_stage_award();
    }
}

void asst::RoguelikeFoldartalGainTaskPlugin::enter_next_floor()
{
    auto foldartal_floor = m_config->get_foldartal_floor();

    // 到达第二层后，获得上一层预见的密文板
    if (foldartal_floor) {
        gain_foldartal(*foldartal_floor);
    }

    foldartal_floor.reset();
    OCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info(m_config->get_theme() + "@Roguelike@FoldartalGainOcrNextLevel");
    if (analyzer.analyze()) {
        std::string foldartar_will_get_next_floor = analyzer.get_result().front().text;
        foldartal_floor = foldartar_will_get_next_floor;
        auto info = basic_info_with_what("FoldartalGainOcrNextLevel");
        info["details"]["foldartal"] = foldartar_will_get_next_floor;
        callback(AsstMsg::SubTaskExtraInfo, info);

        if (m_config->get_first_floor_foldartal()) {
            Log.info("Foldartal will get next floor:", foldartar_will_get_next_floor);
            if (foldartar_will_get_next_floor == m_config->get_start_floor_foldartal()) {
                m_task_ptr->set_enable(false);
            }
            else {
                ProcessTask(*this, { m_config->get_theme() + "@Roguelike@NextLevel" })
                    .set_times_limit("Roguelike@StrategyChange", 0)
                    .run();
                ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ExitThenAbandon" })
                    .set_times_limit("Roguelike@Abandon", 0)
                    .run();
            }
        }
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
