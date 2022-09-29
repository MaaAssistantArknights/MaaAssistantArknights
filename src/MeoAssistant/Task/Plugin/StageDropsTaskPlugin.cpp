#include "StageDropsTaskPlugin.h"

#include <chrono>
#include <regex>
#include <thread>

#include "../Sub/ProcessTask.h"
#include "../Sub/ReportDataTask.h"
#include "Controller.h"
#include "ImageAnalyzer/StageDropsImageAnalyzer.h"
#include "Resource/ItemConfiger.h"
#include "Resource/StageDropsConfiger.h"
#include "RuntimeStatus.h"
#include "TaskData.h"
#include "Utils/AsstUtils.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Version.h"

bool asst::StageDropsTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }
    const std::string task = details.at("details").at("task").as_string();
    if (task == "EndOfAction") {
        auto pre_time_opt = m_status->get_number("LastStartButton2");
        int64_t pre_start_time = pre_time_opt ? pre_time_opt.value() : 0;
        auto pre_reg_time_opt = m_status->get_number("LastRecognizeDrops");
        int64_t pre_recognize_time = pre_reg_time_opt ? pre_reg_time_opt.value() : 0;
        if (pre_start_time + RecognitionTimeOffset == pre_recognize_time) {
            Log.info("Recognitions time too close, pass", pre_start_time, pre_recognize_time);
            return false;
        }
        m_is_annihilation = false;
        return true;
    }
    else if (task == "EndOfActionAnnihilation") {
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

bool asst::StageDropsTaskPlugin::set_enable_penguid(bool enable)
{
    m_enable_penguid = enable;
    return true;
}

bool asst::StageDropsTaskPlugin::set_penguin_id(std::string id)
{
    m_penguin_id = std::move(id);
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
        return false;
    }
    if (need_exit()) {
        return false;
    }
    drop_info_callback();

    if (!check_stage_valid() || check_specify_quantity()) {
        stop_task();
    }

    if (m_enable_penguid && !m_is_annihilation) {
        upload_to_penguin();
    }

    return true;
}

bool asst::StageDropsTaskPlugin::recognize_drops()
{
    LogTraceFunction;

    sleep(Task.get("PRTS")->rear_delay);
    if (need_exit()) {
        return false;
    }

    StageDropsImageAnalyzer analyzer(m_ctrler->get_image());
    if (!analyzer.analyze()) {
        auto info = basic_info();
        info["subtask"] = "RecognizeDrops";
        info["why"] = "掉落识别错误";
        callback(AsstMsg::SubTaskError, info);
        return false;
    }

    auto&& [code, difficulty] = analyzer.get_stage_key();
    m_stage_code = std::move(code);
    m_stage_difficulty = difficulty;
    m_stars = analyzer.get_stars();
    m_cur_drops = analyzer.get_drops();

    if (m_is_annihilation) {
        return true;
    }

    auto last_time_opt = m_status->get_number("LastStartButton2");
    auto last_time = last_time_opt ? last_time_opt.value() : 0;
    m_status->set_number("LastRecognizeDrops", last_time + RecognitionTimeOffset);

    return true;
}

void asst::StageDropsTaskPlugin::drop_info_callback()
{
    LogTraceFunction;

    std::vector<json::value> drops_vec;
    for (const auto& drop : m_cur_drops) {
        json::value info;
        info["itemId"] = drop.item_id;
        info["quantity"] = drop.quantity;
        m_drop_stats[drop.item_id] += drop.quantity;
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
    LogTraceFunction;

    if (m_is_annihilation) {
        return;
    }

    if (!m_start_button_delay_is_set) {
        auto last_time_opt = m_status->get_number("LastStartButton2");
        int64_t pre_start_time = last_time_opt ? last_time_opt.value() : 0;

        if (pre_start_time > 0) {
            m_start_button_delay_is_set = true;
            int64_t duration = time(nullptr) - pre_start_time;
            int elapsed = Task.get("EndOfAction")->pre_delay + Task.get("PRTS")->rear_delay;
            int64_t delay = duration * 1000 - elapsed;
            m_cast_ptr->set_rear_delay("StartButton2", static_cast<int>(delay));
        }
    }
}

void asst::StageDropsTaskPlugin::upload_to_penguin()
{
    LogTraceFunction;

    json::value cb_info = basic_info();
    cb_info["subtask"] = "ReportToPenguinStats";

    // Doc: https://developer.penguin-stats_vec.io/public-api/api-v2-instruction/report-api
    std::string stage_id = m_cur_info_json.get("stage", "stageId", std::string());
    if (stage_id.empty()) {
        cb_info["why"] = "未知关卡";
        cb_info["details"] = json::object { { "stage_code", m_stage_code } };
        callback(AsstMsg::SubTaskError, cb_info);
        return;
    }
    if (m_stars != 3) {
        cb_info["why"] = "非三星作战";
        callback(AsstMsg::SubTaskError, cb_info);
        return;
    }
    json::value body;
    body["server"] = m_server;
    body["stageId"] = stage_id;
    auto& all_drops = body["drops"];
    for (const auto& drop : m_cur_info_json["drops"].as_array()) {
        static const std::array<std::string, 4> filter = {
            "NORMAL_DROP",
            "EXTRA_DROP",
            "FURNITURE",
            "SPECIAL_DROP",
        };
        std::string drop_type = drop.at("dropType").as_string();
        if (ranges::find(filter, drop_type) == filter.cend()) {
            continue;
        }
        if (drop.at("itemId").as_string().empty()) {
            cb_info["why"] = "存在未知掉落";
            callback(AsstMsg::SubTaskError, cb_info);
            return;
        }
        json::value format_drop = drop;
        format_drop.as_object().erase("itemName");
        all_drops.array_emplace(std::move(format_drop));
    }
    body["source"] = "MeoAssistant";
    body["version"] = Version;

    std::string extra_param;
    if (!m_penguin_id.empty()) {
        extra_param = "-H \"authorization: PenguinID " + m_penguin_id + "\"";
    }

    if (!m_report_penguin_task_ptr) {
        m_report_penguin_task_ptr =
            std::make_shared<ReportDataTask>(report_penguin_callback, static_cast<void*>(this), m_task_chain);
    }

    m_report_penguin_task_ptr->set_report_type(ReportType::PenguinStats)
        .set_body(body.to_string())
        .set_extra_param(extra_param)
        .set_retry_times(5)
        .run();
}

void asst::StageDropsTaskPlugin::report_penguin_callback(AsstMsg msg, const json::value& detail, void* custom_arg)
{
    LogTraceFunction;

    auto p_this = static_cast<StageDropsTaskPlugin*>(custom_arg);
    if (!p_this) {
        return;
    }

    if (msg == AsstMsg::SubTaskExtraInfo && detail.get("what", std::string()) == "PenguinId") {
        std::string id = detail.get("details", "id", std::string());
        p_this->m_penguin_id = id;
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
