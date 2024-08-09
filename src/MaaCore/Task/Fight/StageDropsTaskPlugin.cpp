#include "StageDropsTaskPlugin.h"

#include <chrono>
#include <regex>
#include <thread>

#include "Common/AsstTypes.h"
#include "Common/AsstVersion.h"
#include "Config/GeneralConfig.h"
#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/Miscellaneous/StageDropsConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Task/ReportDataTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/StageDropsImageAnalyzer.h"

bool asst::StageDropsTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }
    const std::string task = details.get("details", "task", "");
    if (task == "Fight@EndOfAction") {
        int64_t last_start_time = status()->get_number(LastStartTimeKey).value_or(0);
        int64_t last_recognize_flag = status()->get_number(RecognitionRestrictionsKey).value_or(0);
        if (last_start_time + RecognitionTimeOffset == last_recognize_flag) {
            Log.warn("Only one recognition per start", last_start_time, last_recognize_flag);
            return false;
        }
        m_is_annihilation = false;
        return true;
    }
    else if (task == "Fight@EndOfActionAnnihilation") {
        m_is_annihilation = true;
        return true;
    }
    else {
        return false;
    }
}

void asst::StageDropsTaskPlugin::set_task_ptr(AbstractTask* ptr)
{
    AbstractTaskPlugin::set_task_ptr(ptr);
    m_cast_ptr = dynamic_cast<ProcessTask*>(ptr);
}

bool asst::StageDropsTaskPlugin::set_enable_penguin(bool enable)
{
    m_enable_penguin = enable;
    return true;
}

bool asst::StageDropsTaskPlugin::set_penguin_id(std::string id)
{
    m_penguin_id = std::move(id);
    return true;
}

bool asst::StageDropsTaskPlugin::set_enable_yituliu(bool enable)
{
    m_enable_yituliu = enable;
    return true;
}

bool asst::StageDropsTaskPlugin::set_server(std::string server)
{
    m_server = std::move(server);
    return true;
}

bool asst::StageDropsTaskPlugin::set_specify_quantity(std::unordered_map<std::string, int> quantity)
{
    m_specify_quantity = std::move(quantity);
    return true;
}

bool asst::StageDropsTaskPlugin::_run()
{
    LogTraceFunction;

    set_start_button_delay();

    if (!recognize_drops()) {
        if (!check_stage_valid()) {
            stop_task();
        }
        return false;
    }
    if (need_exit()) {
        return false;
    }
    drop_info_callback();

    if (!check_stage_valid() || check_specify_quantity()) {
        stop_task();
    }

    if (m_is_annihilation) {
        Log.info(__FUNCTION__, "Annihilation is not supported by PenguinStats");
        Log.info(__FUNCTION__, "Annihilation is not supported by Yituliu");
        return true;
    }

    if (m_enable_penguin) {
        if (!upload_to_penguin()) {
            Log.error(__FUNCTION__, "upload_to_penguin failed");
            save_img(utils::path("debug") / utils::path("drops"));
        }
    }
    else {
        Log.info(__FUNCTION__, "PenguinStats is disabled");
    }

    if (m_enable_yituliu) {
        upload_to_yituliu();
    }
    else {
        Log.info(__FUNCTION__, "Yituliu is disabled");
    }

    return true;
}

bool asst::StageDropsTaskPlugin::recognize_drops()
{
    LogTraceFunction;

    sleep(Task.get("PRTS")->post_delay);
    if (need_exit()) {
        return false;
    }

    StageDropsImageAnalyzer analyzer(ctrler()->get_image());

    bool ret = false;
    auto append_image = [&]() {
        while (!need_exit()) {
            ret = false;

            // more materials to reveal?

            auto swipe_begin = Point { WindowWidthDefault - 240, 632 };

            const int swipe_dist = 200;
            ctrler()->swipe(swipe_begin, swipe_begin + swipe_dist * Point::left(), 500, true, 2, 0);
            sleep(Config.get_options().task_delay * 3);

            auto new_img = ctrler()->get_image();

            const auto offset_opt = analyzer.merge_image(new_img);
            if (!offset_opt.has_value()) {
                break;
            }
            const auto offset = offset_opt.value();
            Log.trace("new image offset:", offset);
            if (offset <= 4) {
                ret = true;
                break;
            }
        }
    };

    if (!analyzer.analyze_baseline() || analyzer.get_baseline().empty()) {
        append_image();
    }
    else {
        const auto baseline = analyzer.get_baseline().back().first;
        if (baseline.x + baseline.width >= WindowWidthDefault * 0.95) {
            append_image();
        }
        else {
            ret = true;
        }
    }

    // image strip constructed, start main step
    ret &= analyzer.analyze();

    auto&& [code, difficulty] = analyzer.get_stage_key();
    m_stage_code = std::move(code);
    ranges::transform(m_stage_code, m_stage_code.begin(),
                      [](char ch) -> char { return static_cast<char>(::toupper(ch)); });
    m_stage_difficulty = difficulty;
    m_stars = analyzer.get_stars();
    m_cur_drops = analyzer.get_drops();
    m_times = analyzer.get_times();

    if (!ret) {
        auto info = basic_info();
        info["subtask"] = "RecognizeDrops";
        info["why"] = "掉落识别错误";
        callback(AsstMsg::SubTaskError, info);
        return false;
    }

    if (m_is_annihilation) {
        return true;
    }

    int64_t last_start_time = status()->get_number(LastStartTimeKey).value_or(0);
    int64_t recognize_flag = last_start_time + RecognitionTimeOffset;
    status()->set_number(RecognitionRestrictionsKey, recognize_flag);

    return true;
}

void asst::StageDropsTaskPlugin::drop_info_callback()
{
    LogTraceFunction;

    std::unordered_map<std::string, int> cur_drops_count;
    std::vector<json::value> drops_vec;
    for (const auto& drop : m_cur_drops) {
        m_drop_stats[drop.item_id] += drop.quantity;
        cur_drops_count.emplace(drop.item_id, drop.quantity);
        json::value info;
        info["itemId"] = drop.item_id;
        info["quantity"] = drop.quantity;
        info["itemName"] = drop.item_name;
        info["dropType"] = drop.drop_type_name;
        drops_vec.emplace_back(std::move(info));
    }

    std::vector<json::value> stats_vec;
    for (auto&& [id, count] : m_drop_stats) {
        json::value info;
        info["itemId"] = id;
        const std::string& name = ItemData.get_item_name(id);
        info["itemName"] = name.empty() ? id : name;
        info["quantity"] = count;
        if (auto iter = cur_drops_count.find(id); iter != cur_drops_count.end()) {
            info["addQuantity"] = iter->second;
        }
        else {
            info["addQuantity"] = 0;
        }
        stats_vec.emplace_back(std::move(info));
    }
    //// 排个序，数量多的放前面
    // std::sort(stats_vec.begin(), stats_vec.end(),
    //     [](const json::value& lhs, const json::value& rhs) -> bool {
    //         return lhs.at("count").as_integer() > rhs.at("count").as_integer();
    //     });

    json::value info = basic_info_with_what("StageDrops");
    json::value& details = info["details"];
    details["stars"] = m_stars;
    if (m_times >= 0) {
        details["cur_times"] = m_times;
    }
    details["stats"] = json::array(std::move(stats_vec));
    details["drops"] = json::array(std::move(drops_vec));
    json::value& stage = details["stage"];
    stage["stageCode"] = m_stage_code;
    if (!m_stage_code.empty()) {
        stage["stageId"] = StageDrops.get_stage_info(m_stage_code, m_stage_difficulty).stage_id;
    }

    callback(AsstMsg::SubTaskExtraInfo, info);
    m_cur_info_json = std::move(details);
}

void asst::StageDropsTaskPlugin::set_start_button_delay()
{
    if (m_is_annihilation) {
        return;
    }
    if (m_start_button_delay_is_set) {
        return;
    }

    int64_t last_start_time = status()->get_number(LastStartTimeKey).value_or(0);
    if (last_start_time == 0) {
        return;
    }

    int64_t delay = -1;
    int64_t last_prts1_time = status()->get_number(LastPRTS1TimeKey).value_or(-1);
    if (last_prts1_time == -1) {
        int64_t duration = time(nullptr) - last_start_time;
        int elapsed = Task.get("EndOfAction")->pre_delay + Task.get("PRTS")->post_delay;
        delay = duration * 1000 - elapsed;
    }
    else {
        delay = (last_prts1_time - last_start_time) * 1000;
    }

    if (delay == -1) {
        return;
    }

    m_start_button_delay_is_set = true;
    Log.info(__FUNCTION__, "set StartButton2WaitTime post delay", delay);
    m_cast_ptr->set_post_delay("StartButton2WaitTime", static_cast<int>(delay));
}

bool asst::StageDropsTaskPlugin::upload_to_server(const std::string& subtask, ReportType report_type)
{
    LogTraceFunction;

    if (m_server != "CN" && m_server != "US" && m_server != "JP" && m_server != "KR") {
        return true;
    }

    json::value cb_info = basic_info();
    cb_info["subtask"] = subtask;

    std::string stage_id = m_cur_info_json.get("stage", "stageId", std::string());
    if (stage_id.empty()) {
        cb_info["why"] = "UnknownStage";
        cb_info["details"] =
            json::object { { "stage_code", m_stage_code }, { "stage_difficulty", enum_to_string(m_stage_difficulty) } };
        callback(AsstMsg::SubTaskError, cb_info);
        return false;
    }
    if (m_stars != 3) {
        cb_info["why"] = "NotThreeStars";
        callback(AsstMsg::SubTaskError, cb_info);
        return false;
    }
    if (m_times == -2) {
        cb_info["why"] = "UnknownTimes";
        callback(AsstMsg::SubTaskError, cb_info);
        return false;
    }

    json::value body;
    body["server"] = m_server;
    body["stageId"] = stage_id;
    if (m_times >= 0) { // -1 means not found, don't have times field
        body["times"] = m_times;
    }
    auto& all_drops = body["drops"];
    for (const auto& drop : m_cur_info_json["drops"].as_array()) {
        static const std::array<std::string, 4> filter = {
            "NORMAL_DROP",
            "EXTRA_DROP",
            "FURNITURE",
            "SPECIAL_DROP",
        };
        std::string drop_type = drop.at("dropType").as_string();
        if (drop_type == "UNKNOWN_DROP") {
            cb_info["why"] = "UnknownDropType";
            callback(AsstMsg::SubTaskError, cb_info);
            return false;
        }
        if (ranges::find(filter, drop_type) == filter.cend()) {
            continue;
        }
        if (drop.at("itemId").as_string().empty()) {
            cb_info["why"] = "UnknownDrops";
            callback(AsstMsg::SubTaskError, cb_info);
            return false;
        }
        json::value format_drop = drop;
        format_drop.as_object().erase("itemName");
        all_drops.emplace(std::move(format_drop));
    }
    body["source"] = UploadDataSource;
    body["version"] = Version;

    std::unordered_map<std::string, std::string> extra_headers;
    if (!m_penguin_id.empty()) {
        extra_headers.insert({ "authorization", "PenguinID " + m_penguin_id });
    }

    std::shared_ptr<ReportDataTask> report_task_ptr;
    if (report_type == ReportType::PenguinStats) {
        if (!m_report_penguin_task_ptr) {
            m_report_penguin_task_ptr = std::make_shared<ReportDataTask>(report_penguin_callback, this);
        }
        report_task_ptr = m_report_penguin_task_ptr;
    }
    else if (report_type == ReportType::YituliuBigDataStageDrops) {
        if (!m_report_yituliu_task_ptr) {
            m_report_yituliu_task_ptr = std::make_shared<ReportDataTask>(report_yituliu_callback, this);
        }
        report_task_ptr = m_report_yituliu_task_ptr;
    }

    report_task_ptr->set_report_type(report_type)
        .set_body(body.to_string())
        .set_extra_headers(extra_headers)
        .set_retry_times(5)
        .run();

    return true;
}

bool asst::StageDropsTaskPlugin::upload_to_penguin()
{
    LogTraceFunction;

    const bool result = upload_to_server("ReportToPenguinStats", ReportType::PenguinStats);
    return result;
}

void asst::StageDropsTaskPlugin::report_penguin_callback(AsstMsg msg, const json::value& detail, AbstractTask* task_ptr)
{
    LogTraceFunction;

    auto p_this = dynamic_cast<StageDropsTaskPlugin*>(task_ptr);
    if (!p_this) {
        return;
    }

    if (msg == AsstMsg::SubTaskExtraInfo && detail.get("what", std::string()) == "PenguinId") {
        std::string id = detail.get("details", "id", std::string());
        p_this->m_penguin_id = id;
    }

    p_this->callback(msg, detail);
}

void asst::StageDropsTaskPlugin::upload_to_yituliu()
{
    LogTraceFunction;

    upload_to_server("ReportToYituliu", ReportType::YituliuBigDataStageDrops);
}

void asst::StageDropsTaskPlugin::report_yituliu_callback(AsstMsg msg, const json::value& detail, AbstractTask* task_ptr)
{
    LogTraceFunction;

    auto p_this = dynamic_cast<StageDropsTaskPlugin*>(task_ptr);
    if (!p_this) {
        return;
    }

    p_this->callback(msg, detail);
}

bool asst::StageDropsTaskPlugin::check_stage_valid()
{
    LogTraceFunction;
    static const std::string invalid_stage_code = "_INVALID_";

    if (m_stage_code == invalid_stage_code) {
        json::value info = basic_info();
        info["subtask"] = "CheckStageValid";
        info["why"] = "无奖励关卡";
        callback(AsstMsg::SubTaskError, info);

        return false;
    }
    return true;
}

bool asst::StageDropsTaskPlugin::check_specify_quantity() const
{
    for (const auto& [id, quantity] : m_specify_quantity) {
        if (auto find_iter = m_drop_stats.find(id); find_iter != m_drop_stats.end() && find_iter->second >= quantity) {
            return true;
        }
    }
    return false;
}

void asst::StageDropsTaskPlugin::stop_task()
{
    m_cast_ptr->set_times_limit("StartButton1", 0)
        .set_times_limit("StartButton2", 0)
        .set_times_limit("MedicineConfirm", 0)
        .set_times_limit("StoneConfirm", 0);
}
