#include "RoguelikeCollapsalParadigmTaskPlugin.h"
#include "Config/Roguelike/RoguelikeCollapsalParadigmConfig.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"
#include "Vision/Matcher.h"

#include <algorithm>

#define DEBUG false

bool asst::RoguelikeCollapsalParadigmTaskPlugin::verify(const AsstMsg msg, const json::value& details) const
{
    if ((msg != AsstMsg::SubTaskStart && msg != AsstMsg::SubTaskCompleted) || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (!RoguelikeConfig::is_valid_theme(m_config->get_theme())) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }

    if (m_config->get_theme() != RoguelikeTheme::Sami || !m_config->get_check_clp_pds())  {
        return false;
    }

    const auto task_name = details.get("details", "task", "");

    if (msg == AsstMsg::SubTaskStart && m_banner_triggers_start.find(task_name) != m_banner_triggers_start.end()) {
        m_check_banner = true;
        return true;
    }

    if (msg == AsstMsg::SubTaskCompleted && m_banner_triggers_completed.find(task_name) != m_banner_triggers_completed.end()) {
        m_check_banner = true;
        return true;
    }

    if(msg == AsstMsg::SubTaskStart && m_panel_triggers.find(task_name) != m_panel_triggers.end()) {
        if (new_zone()) {
            m_need_check_panel = true;
            m_verification_check = false;
        }
        
        if (m_need_check_panel) {
            m_need_check_panel = false;
            m_check_panel = true;
            return true;
        }
    }
    
    if (!m_need_check_panel && m_config->get_double_check_clp_pds() && msg == AsstMsg::SubTaskCompleted
        && m_panel_triggers.find(task_name) == m_panel_triggers.end()) {
        m_need_check_panel = true;
        m_verification_check = true;
    };

    return false;
}

bool asst::RoguelikeCollapsalParadigmTaskPlugin::_run()
{
    if (DEBUG) { clp_pd_callback("Current Zone: " + m_zone, 0, "CollapsalParadigmInfo"); }
    if (m_check_banner) {
        if (DEBUG) { clp_pd_callback("Banner Check", 0, "CollapsalParadigmInfo"); }
        m_check_banner = false;
        return check_collapsal_paradigm_banner();
    }
    if (m_check_panel) {
        if (DEBUG) { clp_pd_callback(std::string("Panel Check") + (m_verification_check ? " (Verification)" : ""), 0, "CollapsalParadigmInfo"); }
        m_check_panel = false;
        return check_collapsal_paradigm_panel();
    }
    return false;
}

bool asst::RoguelikeCollapsalParadigmTaskPlugin::check_collapsal_paradigm_banner()
{
    const std::string& theme = m_config->get_theme();

    wait_for_loading(200);

    OCRer analyzer(ctrler()->get_image());
    if (DEBUG) { analyzer.save_img(utils::path("debug") / utils::path("collapsalParadigms")); }
    analyzer.set_task_info(theme + "@Roguelike@CheckCollapsalParadigms_banner");
    auto detected = analyzer.analyze();
    if (!detected) { // 以防万一，再识别一次
        wait_for_loading(200);
        analyzer.set_image(ctrler()->get_image());
        if (DEBUG) { analyzer.save_img(utils::path("debug") / utils::path("collapsalParadigms")); }
        detected = analyzer.analyze();
    }
    if (detected) {
        const std::vector<CollapsalParadigmClass>& clp_pd_classes = RoguelikeCollapsalParadigms.get_clp_pd_classes(theme);
        const std::unordered_map<std::string, unsigned int>& clp_pd_dict = RoguelikeCollapsalParadigms.get_clp_pd_dict(theme);
        const std::unordered_set<std::string>& expected_clp_pds = m_config->get_expected_clp_pds();
        std::vector<std::string> prev_clp_pds = m_config->get_clp_pds();
        
        std::string_view deepen_text = (Task.get<OcrTaskInfo>(theme + "@Roguelike@CheckCollapsalParadigms_banner")->text)[0]; // "坍缩范式加深"
        std::string_view weaken_text = (Task.get<OcrTaskInfo>(theme + "@Roguelike@CheckCollapsalParadigms_banner")->text)[1]; // "坍缩范式消退"

        int banner_y = 0;
        int deepen_or_weaken = 0; // 1 = deepen; 0 = unknown; -1 = weaken;
        std::string cur_clp_pd;
        std::vector<std::string>::iterator it;

        OCRer::ResultsVec ocr_results = analyzer.get_result();
        std::sort(ocr_results.begin(), ocr_results.end(),
            [](const OCRer::Result& x1, const OCRer::Result& x2){ return x1.rect.y < x2.rect.y; });
        for (const OCRer::Result& result : ocr_results) {
            if (!deepen_or_weaken) {
                if (result.text != deepen_text && result.text != weaken_text) {
                    clp_pd_callback("Erroneous Recognition in Banner Check" , 0, "CollapsalParadigmError");
                    continue;
                }
                if (result.text == deepen_text) {
                    deepen_or_weaken = 1;
                }
                else {
                    deepen_or_weaken = -1;
                }
                banner_y = result.rect.y + result.rect.height;
            } else {
                if (result.text == deepen_text || result.text == weaken_text
                    || result.rect.y >= banner_y + 25) {
                    clp_pd_callback("Erroneous Recognition in Banner Check" , 0, "CollapsalParadigmError");
                    continue;
                }
                cur_clp_pd = result.text;
                const CollapsalParadigmClass& cur_class = clp_pd_classes.at(clp_pd_dict.at(cur_clp_pd));
                if (deepen_or_weaken == 1) {
                    it = std::find(prev_clp_pds.begin(), prev_clp_pds.end(), cur_clp_pd);
                    if (it == prev_clp_pds.end()) {
                        if (cur_clp_pd == cur_class.level_2) {
                            it = std::find(prev_clp_pds.begin(), prev_clp_pds.end(), cur_class.level_1);
                            if (it != prev_clp_pds.end()) {
                                clp_pd_callback(cur_clp_pd, 1, *it);
                                *it = cur_clp_pd;
                            }
                            else {
                                clp_pd_callback(cur_clp_pd, 1);
                                prev_clp_pds.emplace_back(cur_clp_pd);
                            }
                        } else {
                            clp_pd_callback(cur_clp_pd, 1);
                            prev_clp_pds.emplace_back(cur_clp_pd);
                        }
                        // check whether cur_clp_pd is an expected collapsal paradigm
                        // 判断 cur_clp_pd 是否为需要的坍缩范式
                        if (expected_clp_pds.find(cur_clp_pd) != expected_clp_pds.end()) {
                            exit_then_stop();
                            return true;
                        } else if (m_config->get_mode() == RoguelikeMode::CLP_PDS) { // 刷坍缩范式模式下，获得了不想要的坍缩范式
                            exit_then_restart();
                            return true;
                        }
                    } else {
                        clp_pd_callback("Duplicate Recognition in Banner Check" , 0, "CollapsalParadigmInfo");
                    }
                } else {
                    it = std::find(prev_clp_pds.begin(), prev_clp_pds.end(), cur_clp_pd);
                    if (it != prev_clp_pds.end()) {
                        if (cur_clp_pd == cur_class.level_2) {
                            clp_pd_callback(cur_class.level_1, -1, cur_clp_pd);
                            *it = cur_class.level_1;
                            // 如果2级直接消退到0级的话会有几次提示？这里可能会出问题，谨慎一些，再检查一次。
                            m_need_check_panel = true;
                        }
                        else {
                            clp_pd_callback("", -1, cur_clp_pd);
                            prev_clp_pds.erase(it);
                        }
                    }
                    else {
                        clp_pd_callback("Duplicate Recognition in Banner Check" , 0, "CollapsalParadigmInfo");
                    }
                }
                deepen_or_weaken = 0;
            }
        }
        m_config->set_clp_pds(std::move(prev_clp_pds));
    }
    return true;
}

bool asst::RoguelikeCollapsalParadigmTaskPlugin::check_collapsal_paradigm_panel()
{   
    const std::string& theme = m_config->get_theme();

    const std::vector<CollapsalParadigmClass>& clp_pd_classes = RoguelikeCollapsalParadigms.get_clp_pd_classes(theme);
    const std::unordered_map<std::string, unsigned int>& clp_pd_dict = RoguelikeCollapsalParadigms.get_clp_pd_dict(theme);
    const std::unordered_set<std::string>& expected_clp_pds = m_config->get_expected_clp_pds();
    std::vector<std::string> prev_clp_pds = m_config->get_clp_pds();
    std::vector<std::string>::iterator it = prev_clp_pds.begin();

    wait_for_stage(200);

    OCRer analyzer;
    analyzer.set_task_info(theme + "@Roguelike@CheckCollapsalParadigms_panel");
    do {
        toggle_collapsal_status_panel();
        analyzer.set_image(ctrler()->get_image());
    } while (!analyzer.analyze()); // 即便没有坍缩范式也应当检测到“当前坍缩值”
    if (DEBUG) { analyzer.save_img(utils::path("debug") / utils::path("collapsalParadigms")); }

    OCRer::ResultsVec ocr_results = analyzer.get_result();
    // 识别到两个及以上坍缩范式的时候，向上滑动一下再识别一次
    // if detect more than three collapsal paradigms, swipe up and analyze again
    if (ocr_results.size() >= 3) {
        ctrler()->swipe(m_swipe_begin, m_swipe_end, 500);
        sleep(500);
        analyzer.set_image(ctrler()->get_image());
        if (DEBUG) { analyzer.save_img(utils::path("debug") / utils::path("collapsalParadigms")); }
        if (analyzer.analyze()) {
            ocr_results = analyzer.get_result();
        }
    }

    std::sort(ocr_results.begin(), ocr_results.end(),
        [](const OCRer::Result& x1, const OCRer::Result& x2){ return x1.rect.y < x2.rect.y; });
    
    std::vector<std::string> cur_clp_pds;
    std::transform(ocr_results.begin() + 1, ocr_results.end(), // 第一个结果肯定是“当前坍缩值”
        std::back_inserter(cur_clp_pds),
        [](OCRer::Result x) { return x.text; });
    
    if (m_verification_check && prev_clp_pds != cur_clp_pds) {
        clp_pd_callback("Collapsal Paradigm Verification Failed", 0, "CollapsalParadigmError");
    }

    for (const std::string& cur_clp_pd : cur_clp_pds) {
        // 判断坍缩范式变动
        const unsigned int& cur_index = clp_pd_dict.at(cur_clp_pd);
        const CollapsalParadigmClass& cur_class = clp_pd_classes.at(cur_index);
        while (it != prev_clp_pds.end() && clp_pd_dict.at(*it) != cur_index) {
            clp_pd_callback("", -1, *it);
            ++it;
        }
        if (it == prev_clp_pds.end()) { // 全新的坍缩范式
            clp_pd_callback(cur_clp_pd, 1);
        } else {
            if (cur_clp_pd == cur_class.level_2 && *it == cur_class.level_1) { // 从1级加深到2级
                clp_pd_callback(cur_clp_pd, 1, cur_class.level_1);

            } else if (cur_clp_pd == cur_class.level_1 && *it == cur_class.level_2) { // 从2级消退到1级
                clp_pd_callback(cur_clp_pd, -1, cur_class.level_2);
            }
            ++it;
        }
        // 判断 cur_clp_pd 是否为需要的坍缩范式
        // check whether cur_clp_pd is an expected collapsal paradigm
        if (expected_clp_pds.find(cur_clp_pd) != expected_clp_pds.end()) {
            toggle_collapsal_status_panel();
            exit_then_stop();
            return true;
        }
        else if (m_config->get_mode() == RoguelikeMode::CLP_PDS) { // 刷坍缩范式模式下，获得了不想要的坍缩范式
            toggle_collapsal_status_panel();
            exit_then_restart();
            return true;
        }
    }
    while (it != prev_clp_pds.end()) {
        clp_pd_callback("", -1, *it);
        ++it;
    }

    m_config->set_clp_pds(std::move(cur_clp_pds));
    
    toggle_collapsal_status_panel();

    return true;
}

void asst::RoguelikeCollapsalParadigmTaskPlugin::exit_then_stop()
{
    ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ExitThenAbandon" })
        .set_times_limit(m_config->get_theme() + "@Roguelike@Abandon", 0)
        .run();
    m_task_ptr->set_enable(false);
}

void asst::RoguelikeCollapsalParadigmTaskPlugin::exit_then_restart()
{
    ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ExitThenAbandon" })
        .set_times_limit("Roguelike@Abandon", 0)
        .run();
}

void asst::RoguelikeCollapsalParadigmTaskPlugin::clp_pd_callback(std::string cur, int deepen_or_weaken, std::string prev)
{
    auto info = basic_info_with_what("RoguelikeCollapsalParadigms");
    info["details"]["cur"] = cur;
    info["details"]["deepen_or_weaken"] = deepen_or_weaken;
    info["details"]["prev"] = prev;
    callback(AsstMsg::SubTaskExtraInfo, info);
}

void asst::RoguelikeCollapsalParadigmTaskPlugin::toggle_collapsal_status_panel()
{
    ctrler()->click(m_roi); // 点击屏幕上方居中区域以开始检查坍缩状态
    sleep(500);
}

void asst::RoguelikeCollapsalParadigmTaskPlugin::wait_for_loading(unsigned int millisecond)
{
    OCRer analyzer(ctrler()->get_image());
    // if (DEBUG) { analyzer.save_img(utils::path("debug") / utils::path("collapsalParadigms")); }
    analyzer.set_task_info(m_config->get_theme() + "@Roguelike@CheckCollapsalParadigms_loading");
    while (analyzer.analyze()) {
        sleep(100);
        analyzer.set_image(ctrler()->get_image());
        // if (DEBUG) { analyzer.save_img(utils::path("debug") / utils::path("collapsalParadigms")); }
    }
    sleep(millisecond);
}

void asst::RoguelikeCollapsalParadigmTaskPlugin::wait_for_stage(unsigned int millisecond)
{
    Matcher matcher(ctrler()->get_image());
    // if (DEBUG) { analyzer.save_img(utils::path("debug") / utils::path("collapsalParadigms")); }
    matcher.set_task_info(m_config->get_theme() + "@Roguelike@CheckCollapsalParadigms_onStage");
    while (!matcher.analyze()) {
        sleep(100);
        matcher.set_image(ctrler()->get_image());
        // if (DEBUG) { analyzer.save_img(utils::path("debug") / utils::path("collapsalParadigms")); }
    }
    sleep(millisecond);
}

bool asst::RoguelikeCollapsalParadigmTaskPlugin::new_zone() const
{
    Matcher matcher(ctrler()->get_image());
    matcher.set_task_info(m_config->get_theme() + "@Roguelike@CheckCollapsalParadigms_onStage");
    if (matcher.analyze()) {
        std::string zone = m_zone_dict.at(matcher.get_result().templ_name);
        if (zone != m_zone) {
            m_zone = zone;
            return true;
        }
    }
    return false;
}
