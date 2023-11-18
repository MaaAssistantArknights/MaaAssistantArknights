#include "RoguelikeSettlementTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

bool asst::RoguelikeSettlementTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    auto task_name = details.at("details").at("task").as_string();
    if (task_name.ends_with("Roguelike@GamePass")) {
        m_game_pass = true;
        return m_config->get_mode() == RoguelikeMode::Exp;
    }
    else if (task_name.ends_with("Roguelike@MissionFailedFlag2")) {
        m_game_pass = false;
        return m_config->get_mode() == RoguelikeMode::Exp;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeSettlementTaskPlugin::_run()
{
    const static auto task = Task.get("RoguelikeSettlementConfirm");
    auto json_msg = basic_info_with_what("RoguelikeSettlement");
    json_msg["details"]["game_pass"] = m_game_pass;

    sleep(task->special_params[0]);

    if (m_game_pass) {
        save_img(utils::path("achievement") / utils::path("roguelike"));
    }

    const static auto rect = Task.get("Roguelike@ClickToStartPoint")->specific_rect;
    ctrler()->click(rect);
    sleep(task->pre_delay);

    if (!wait_for_whole_page()) {
        Log.error(__FUNCTION__, "wait for whole page failed");
        save_img(utils::path("debug") / utils::path("roguelike"));
        return true;
    }
    sleep(task->post_delay);
    auto image = ctrler()->get_image();

    if (m_game_pass) {
        save_img(utils::path("achievement") / utils::path("roguelike"));
    }
    get_settlement_info(json_msg, image);
    callback(AsstMsg::SubTaskExtraInfo, json_msg);
    return true;
}

bool asst::RoguelikeSettlementTaskPlugin::get_settlement_info(json::value& info, const cv::Mat& image)
{
    auto append_data = [&](const std::string& task_name, const std::string& ocr_result) {
        int num = -1;
        if (!utils::chars_to_number(ocr_result, num)) {
            Log.error(__FUNCTION__, "convert str to int failed, task:", task_name, ", string:", ocr_result);
            return;
        }

        auto tag = task_name.substr(task_name.find("-") + 1);
        utils::tolowers(tag);
        info["details"][tag] = num;
    };

    auto analyze_battle_data = [&](const std::string& task_name) {
        RegionOCRer ocr(image);
        ocr.set_task_info(task_name);
        ocr.set_bin_threshold(0, 255);
        if (!ocr.analyze()) {
            Log.error(__FUNCTION__, "analyze battle data failed, task:", task_name);
            return;
        }
        append_data(task_name, ocr.get_result().text);
    };

    auto analyze_text_data = [&](const std::string& task_name) {
        RegionOCRer ocr(image);
        ocr.set_task_info(m_config->get_theme() + "@" + task_name);
        ocr.set_bin_threshold(50, 255);
        if (!ocr.analyze()) {
            Log.error(__FUNCTION__, "analyze battle data failed, task:", task_name);
            return;
        }
        auto tag = task_name.substr(task_name.find("-") + 1);
        utils::tolowers(tag);
        info["details"][tag] = ocr.get_result().text;
    };

    static const auto battle_task_name =
        std::vector<std::string> { "RoguelikeSettlementOcr-Floor",    "RoguelikeSettlementOcr-Step",
                                   "RoguelikeSettlementOcr-Combat",   "RoguelikeSettlementOcr-Recruit",
                                   "RoguelikeSettlementOcr-Object",   "RoguelikeSettlementOcr-BOSS",
                                   "RoguelikeSettlementOcr-Emergency" };
    static auto text_task_name =
        std::vector<std::string> { "RoguelikeSettlementOcr-Difficulty", "RoguelikeSettlementOcr-Score",
                                   "RoguelikeSettlementOcr-Exp", "RoguelikeSettlementOcr-Skill" };
    if (m_config->get_theme() == "Phantom") {
        text_task_name.pop_back();
    }

    ranges::for_each(battle_task_name, analyze_battle_data);
    ranges::for_each(text_task_name, analyze_text_data);

    return true;
}

bool asst::RoguelikeSettlementTaskPlugin::wait_for_whole_page()
{
    int retry = 0;
    cv::Mat image;
    Matcher matcher;
    matcher.set_task_info("RoguelikeSettlementConfirm");
    do {
        image = ctrler()->get_image();
        matcher.set_image(image);
        if (matcher.analyze()) {
            return true;
        }
        Log.error(__FUNCTION__, "RoguelikeSettlementConfirm match failed, retry:", retry);
        ++retry;
        sleep(Config.get_options().task_delay);
    } while (!need_exit() && retry < 20);
    return false;
}
<<<<<<< HEAD

void asst::RoguelikeSettlementTaskPlugin::save_img(const cv::Mat& image, const std::filesystem::path& relative_dir,
                                                   std::string name)
{
    if (image.empty()) {
        return;
    }

    {
        // 第1次或每执行 debug.clean_files_freq(100) 次后执行清理
        // 限制文件数量 debug.max_debug_file_num
        if (m_save_file_cnt[relative_dir] == 0) {
            filenum_ctrl(relative_dir, Config.get_options().debug.max_debug_file_num);
            m_save_file_cnt[relative_dir] = 0;
        }
        m_save_file_cnt[relative_dir] =
            (m_save_file_cnt[relative_dir] + 1) % Config.get_options().debug.clean_files_freq;
    }

    auto relative_path = relative_dir / (utils::get_time_filestem() + "_" + name + ".png");
    Log.trace("Save image", relative_path);
    asst::imwrite(relative_path, image);
}
=======
>>>>>>> parent of 1f7b6330e (fix: 优化部分情况下肉鸽技结算时技能增长未完全显示导致识别不正确)
