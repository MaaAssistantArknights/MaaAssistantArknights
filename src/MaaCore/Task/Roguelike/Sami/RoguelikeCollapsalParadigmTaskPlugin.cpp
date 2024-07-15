#include "RoguelikeCollapsalParadigmTaskPlugin.h"
#include "Config/Roguelike/Sami/RoguelikeCollapsalParadigmConfig.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"
#include "Vision/Matcher.h"

#include <algorithm>

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
            m_config->set_need_check_panel(true);
            m_verification_check = false;
        }
        
        if (m_config->get_need_check_panel()) {
            m_config->set_need_check_panel(false);
            m_check_panel = true;
            return true;
        }
    }
    
    if (!m_config->get_need_check_panel() && m_config->get_double_check_clp_pds() && msg == AsstMsg::SubTaskCompleted
        && m_panel_triggers.find(task_name) == m_panel_triggers.end()) {
        m_config->set_need_check_panel(true);
        m_verification_check = true;
    };

    return false;
}

bool asst::RoguelikeCollapsalParadigmTaskPlugin::_run()
{
    if (m_check_banner) {
        m_check_banner = false;
        return check_collapsal_paradigm_banner();
    }
    if (m_check_panel) {
        m_check_panel = false;
        return check_collapsal_paradigm_panel();
    }
    return false;
}

bool asst::RoguelikeCollapsalParadigmTaskPlugin::check_collapsal_paradigm_banner()
{
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();

    wait_for_loading(200);

    cv::Mat image = ctrler()->get_image();
    OCRer analyzer1(image); // 检测 "坍缩范式加深" 或 "坍缩范式消退"
#ifdef ASST_DEBUG
    analyzer1.save_img(utils::path("debug") / utils::path("collapsalParadigms"));
#endif
    analyzer1.set_task_info(theme + "@Roguelike@CheckCollapsalParadigms_onBanner");
    auto detected = analyzer1.analyze();
    if (!detected) { // 以防万一，再识别一次
        wait_for_loading(200);
        image = ctrler()->get_image();
        analyzer1.set_image(image);
#ifdef ASST_DEBUG
        analyzer1.save_img(utils::path("debug") / utils::path("collapsalParadigms"));
#endif
        detected = analyzer1.analyze();
    }
    if (detected) {
        const std::vector<CollapsalParadigmClass>& clp_pd_classes = RoguelikeCollapsalParadigms.get_clp_pd_classes(theme);
        const std::unordered_map<std::string, unsigned int>& clp_pd_dict = RoguelikeCollapsalParadigms.get_clp_pd_dict(theme);
        const std::unordered_set<std::string>& expected_clp_pds = m_config->get_expected_clp_pds();
        std::vector<std::string> prev_clp_pds = m_config->get_clp_pds();

        OCRer analyzer2(image); // 检测坍缩范式
        analyzer2.set_task_info(theme + "@Roguelike@CheckCollapsalParadigms_banner");
        if (!analyzer2.analyze()) {
            Log.info("Roguelike Collapsal Paradigm Task Plugin: Erroneous Recognition in Banner Check");
            return false;
        }

        OCRer::ResultsVec ocr_results1 = analyzer1.get_result();
        std::sort(ocr_results1.begin(), ocr_results1.end(), m_compare);
        
        OCRer::ResultsVec ocr_results2 = analyzer2.get_result();
        std::sort(ocr_results2.begin(), ocr_results2.end(), m_compare);
        
        OCRer::ResultsVec::iterator result_it1 = ocr_results1.begin();
        OCRer::ResultsVec::iterator result_it2 = ocr_results2.begin();

        std::vector<std::string>::iterator it;
        while (result_it1 != ocr_results1.end() && result_it2 != ocr_results2.end()) {
            if ((*result_it2).rect.y >= (*result_it1).rect.y + (*result_it1).rect.height + 25) {
                Log.info("Roguelike Collapsal Paradigm Task Plugin: Erroneous Recognition in Banner Check");
                ++result_it1;
                continue;
            }
            else if ((*result_it2).rect.y <= (*result_it1).rect.y) {
                Log.info("Roguelike Collapsal Paradigm Task Plugin: Erroneous Recognition in Banner Check");
                ++result_it2;
                continue;
            }
            std::string cur_clp_pd = (*result_it2).text;
            const CollapsalParadigmClass& cur_class = clp_pd_classes.at(clp_pd_dict.at(cur_clp_pd));
            if ((*result_it1).text == m_deepen_text) {
                if (it = std::find(prev_clp_pds.begin(), prev_clp_pds.end(), cur_clp_pd); it == prev_clp_pds.end()) {
                    if (cur_clp_pd == cur_class.level_2) {
                        if (it = std::find(prev_clp_pds.begin(), prev_clp_pds.end(), cur_class.level_1); it != prev_clp_pds.end()) {
                            clp_pd_callback(cur_clp_pd, 1, *it);
                            *it = cur_clp_pd;
                        }
                        else {
                            clp_pd_callback(cur_clp_pd, 1);
                            prev_clp_pds.emplace_back(cur_clp_pd);
                        }
                    }
                    else {
                        clp_pd_callback(cur_clp_pd, 1);
                        prev_clp_pds.emplace_back(cur_clp_pd);
                    }
                    // check whether cur_clp_pd is an expected collapsal paradigm
                    // 判断 cur_clp_pd 是否为需要的坍缩范式
                    if (expected_clp_pds.find(cur_clp_pd) != expected_clp_pds.end()) {
                        exit_then_stop();
                        return true;
                    }
                    else if (m_config->get_mode() == RoguelikeMode::CLP_PDS) { // 刷坍缩范式模式下，获得了不想要的坍缩范式
                        exit_then_restart();
                        return true;
                    }
                }
            }
            else {
                if (it = std::find(prev_clp_pds.begin(), prev_clp_pds.end(), cur_clp_pd); it != prev_clp_pds.end()) {
                    if (cur_clp_pd == cur_class.level_2) {
                        clp_pd_callback(cur_class.level_1, -1, cur_clp_pd);
                        *it = cur_class.level_1;
                        // 如果2级直接消退到0级的话会有几次提示？这里可能会出问题，谨慎一些，晚些再检查一次。
                        m_config->set_need_check_panel(true);
                    }
                    else {
                        clp_pd_callback("", -1, cur_clp_pd);
                        prev_clp_pds.erase(it);
                    }
                }
            }
            ++result_it1;
            ++result_it2;
        }
        m_config->set_clp_pds(std::move(prev_clp_pds));
    }
    return true;
}

bool asst::RoguelikeCollapsalParadigmTaskPlugin::check_collapsal_paradigm_panel()
{   
    LogTraceFunction;

#ifdef ASST_DEBUG
    if (m_verification_check) {
        Log.info("Roguelike Collapsal Paradigm Task Plugin: Verification");
    }
#endif

    const std::string& theme = m_config->get_theme();

    const std::vector<CollapsalParadigmClass>& clp_pd_classes = RoguelikeCollapsalParadigms.get_clp_pd_classes(theme);
    const std::unordered_map<std::string, unsigned int>& clp_pd_dict = RoguelikeCollapsalParadigms.get_clp_pd_dict(theme);
    const std::unordered_set<std::string>& expected_clp_pds = m_config->get_expected_clp_pds();
    std::vector<std::string> prev_clp_pds = m_config->get_clp_pds();

    wait_for_stage(200);

    cv::Mat image;
    OCRer analyzer;
    analyzer.set_task_info(theme + "@Roguelike@CheckCollapsalParadigms_onPanel"); // 检测 "当前坍缩值" 字样
    do {
        toggle_collapsal_status_panel();
        image = ctrler()->get_image();
        analyzer.set_image(image);
    } while (!analyzer.analyze());
#ifdef ASST_DEBUG
    analyzer.save_img(utils::path("debug") / utils::path("collapsalParadigms"));
#endif

    // 获取当前坍缩范式
    std::vector<std::string> cur_clp_pds;
    analyzer.set_task_info(theme + "@Roguelike@CheckCollapsalParadigms_panel"); // 检测坍缩范式
    if (analyzer.analyze()) {
        OCRer::ResultsVec ocr_results = analyzer.get_result();
        // 识别到两个及以上坍缩范式的时候，向上滑动一下再识别一次
        // if detect more than two collapsal paradigms, swipe up and analyze again
        if (ocr_results.size() >= 2) {
            ctrler()->swipe(m_swipe_begin, m_swipe_end, 500);
            sleep(500);
            analyzer.set_image(ctrler()->get_image());
#ifdef ASST_DEBUG
            analyzer.save_img(utils::path("debug") / utils::path("collapsalParadigms"));
#endif
            if (analyzer.analyze()) {
                ocr_results = analyzer.get_result();
            }
        }
        std::sort(ocr_results.begin(), ocr_results.end(), m_compare);
        
        std::transform(ocr_results.begin(), ocr_results.end(),
            std::back_inserter(cur_clp_pds),
            [](OCRer::Result x) { return x.text; });
    }
    
    if (m_verification_check) {
        if (prev_clp_pds != cur_clp_pds) {
            Log.info("Roguelike Collapsal Paradigm Task Plugin: Verification Failed");
#ifdef ASST_DEBUG
            Log.info("–––––––– Previous ––––––––––––––");
            for (const std::string& clp_pd: prev_clp_pds) {
                Log.info(clp_pd);
            }
            Log.info("–––––––– Current –––––––––––––––");
            for (const std::string& clp_pd: cur_clp_pds) {
                Log.info(clp_pd);
            }
#endif
        }
        else {
            toggle_collapsal_status_panel();
            return true;
        }
    }

    std::vector<std::string>::iterator it = prev_clp_pds.begin();
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
    json::value info = basic_info_with_what("RoguelikeCollapsalParadigms");
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
#ifdef ASST_DEBUG
    analyzer.save_img(utils::path("debug") / utils::path("collapsalParadigms"));
#endif
    analyzer.set_task_info(m_config->get_theme() + "@Roguelike@CheckCollapsalParadigms_loading");
    while (analyzer.analyze()) {
        sleep(100);
        analyzer.set_image(ctrler()->get_image());
#ifdef ASST_DEBUG
        analyzer.save_img(utils::path("debug") / utils::path("collapsalParadigms"));
#endif
    }
    sleep(millisecond);
}

void asst::RoguelikeCollapsalParadigmTaskPlugin::wait_for_stage(unsigned int millisecond)
{
    Matcher matcher(ctrler()->get_image());
#ifdef ASST_DEBUG
    matcher.save_img(utils::path("debug") / utils::path("collapsalParadigms"));
#endif
    matcher.set_task_info(m_config->get_theme() + "@Roguelike@CheckCollapsalParadigms_onStage");
    while (!matcher.analyze()) {
        sleep(100);
        matcher.set_image(ctrler()->get_image());
#ifdef ASST_DEBUG
        matcher.save_img(utils::path("debug") / utils::path("collapsalParadigms"));
#endif
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
#ifdef ASST_DEBUG
            Log.info("Roguelike Collapsal Paradigm Task Plugin: Current Zone is " + m_zone);
#endif
            return true;
        }
    }
    return false;
}
