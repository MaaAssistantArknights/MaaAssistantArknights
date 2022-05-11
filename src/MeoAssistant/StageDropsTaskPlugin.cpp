#include "StageDropsTaskPlugin.h"

#include <thread>
#include <chrono>
#include <regex>

#include "Controller.h"
#include "Resource.h"
#include "ProcessTask.h"
#include "RuntimeStatus.h"
#include "Version.h"
#include "AsstUtils.hpp"
#include "Logger.hpp"
#include "TaskData.h"

bool asst::StageDropsTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted
        || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "EndOfAction") {
        auto pre_time_opt = m_status->get_data("LastStartButton2");
        int64_t pre_start_time = pre_time_opt ? pre_time_opt.value() : 0;
        auto pre_reg_time_opt = m_status->get_data("LastRecognizeDrops");
        int64_t pre_recognize_time = pre_reg_time_opt ? pre_reg_time_opt.value() : 0;
        if (pre_start_time + RecognizationTimeOffset == pre_recognize_time) {
            Log.info("Recognization time too close, pass", pre_start_time, pre_recognize_time);
            return false;
        }
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

bool asst::StageDropsTaskPlugin::_run()
{
    LogTraceFunction;

    set_startbutton_delay();

    if (!recognize_drops()) {
        return false;
    }
    if (need_exit()) {
        return false;
    }
    drop_info_callback();

    check_stage_valid();

    if (m_enable_penguid) {
        auto upload_future = std::async(
            std::launch::async,
            &StageDropsTaskPlugin::upload_to_penguin, this);
        m_upload_pending.emplace_back(std::move(upload_future));
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
    Resrc.penguin().set_language(m_server);

    const cv::Mat image = m_ctrler->get_image(true);
    std::string res = Resrc.penguin().recognize(image);
    Log.trace("Results of penguin recognition:\n", res);

    m_cur_drops = json::parse(res).value();
    // 兼容老版本 json 格式
    auto& drop_area = m_cur_drops["dropArea"];
    m_cur_drops["drops"] = drop_area["drops"];
    m_cur_drops["dropTypes"] = drop_area["dropTypes"];

    auto last_time_opt = m_status->get_data("LastStartButton2");
    auto last_time = last_time_opt ? last_time_opt.value() : 0;
    m_status->set_data("LastRecognizeDrops", last_time + RecognizationTimeOffset);


    return true;
}

void asst::StageDropsTaskPlugin::drop_info_callback()
{
    LogTraceFunction;

    json::value drops_details = m_cur_drops;
    auto& item = Resrc.item();
    for (json::value& drop : drops_details["drops"].as_array()) {
        std::string id = drop["itemId"].as_string();
        int quantity = drop["quantity"].as_integer();
        m_drop_stats[id] += quantity;
        const std::string& name = item.get_item_name(id);
        drop["itemName"] = name.empty() ? "未知材料" : name;
    }
    std::vector<json::value> statistics_vec;
    for (auto&& [id, count] : m_drop_stats) {
        json::value info;
        info["itemId"] = id;
        const std::string& name = item.get_item_name(id);
        info["itemName"] = name.empty() ? "未知材料" : name;
        info["quantity"] = count;
        statistics_vec.emplace_back(std::move(info));
    }
    //// 排个序，数量多的放前面
    //std::sort(statistics_vec.begin(), statistics_vec.end(),
    //    [](const json::value& lhs, const json::value& rhs) -> bool {
    //        return lhs.at("count").as_integer() > rhs.at("count").as_integer();
    //    });

    drops_details["stats"] = json::array(std::move(statistics_vec));

    json::value info = basic_info_with_what("StageDrops");
    info["details"] = drops_details;

    callback(AsstMsg::SubTaskExtraInfo, info);
}

void asst::StageDropsTaskPlugin::set_startbutton_delay()
{
    LogTraceFunction;

    if (!m_startbutton_delay_setted) {
        auto last_time_opt = m_status->get_data("LastStartButton2");;
        int64_t pre_start_time = last_time_opt ? last_time_opt.value() : 0;

        if (pre_start_time > 0) {
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

    auto& opt = Resrc.cfg().get_options();

    json::value info = basic_info();
    info["subtask"] = "ReportToPenguinStats";
    callback(AsstMsg::SubTaskStart, info);

    // Doc: https://developer.penguin-stats.io/public-api/api-v2-instruction/report-api
    std::string stage_id = m_cur_drops.get("stage", "stageId", std::string());
    if (stage_id.empty()) {
        info["why"] = "未知关卡";
        callback(AsstMsg::SubTaskError, info);
        return;
    }
    if (m_cur_drops.get("stars", 0) != 3) {
        info["why"] = "非三星作战";
        callback(AsstMsg::SubTaskError, info);
        return;
    }
    json::value body;
    body["server"] = m_server;
    body["stageId"] = stage_id;
    // To fix: https://github.com/MistEO/MeoAssistantArknights/issues/40
    body["drops"] = json::array();
    for (auto&& drop : m_cur_drops["drops"].as_array()) {
        if (drop["itemId"].as_string().empty()
            || drop["dropType"].as_string() == "LMB") {
            continue;
        }
        body["drops"].as_array().emplace_back(drop);
    }
    body["source"] = "MeoAssistant";
    body["version"] = Version;

    std::string body_escape = utils::string_replace_all(body.to_string(), "\"", "\\\"");
    std::string cmd_line = utils::string_replace_all(opt.penguin_report.cmd_format, "[body]", body_escape);

    std::string extra_param;
    if (!m_penguin_id.empty()) {
        extra_param = "-H \"authorization: PenguinID " + m_penguin_id + "\"";
    }
    cmd_line = utils::string_replace_all(cmd_line, "[extra]", extra_param);

    Log.trace("request_penguin |", cmd_line);

    std::string response = utils::callcmd(cmd_line);

    static const std::regex penguinid_regex(R"(X-Penguin-Set-Penguinid: (\d+))");
    std::smatch penguinid_sm;
    if (std::regex_search(response, penguinid_sm, penguinid_regex)) {
        json::value id_info = basic_info_with_what("PenguinId");
        id_info["details"]["id"] = std::string(penguinid_sm[1]);
        callback(AsstMsg::SubTaskExtraInfo, id_info);
    }

    Log.trace("response:\n", response);

    callback(AsstMsg::SubTaskCompleted, info);
}

bool asst::StageDropsTaskPlugin::check_stage_valid()
{
    LogTraceFunction;

    std::string stage_code = m_cur_drops.get("stage", "stageCode", std::string());

    if (stage_code.find("-EX-") != std::string::npos) {
        json::value info = basic_info();
        info["subtask"] = "CheckStageValid";
        info["why"] = "EX关卡";
        callback(AsstMsg::SubTaskError, info);

        m_cast_ptr->set_times_limit("StartButton1", 0)
            .set_times_limit("StartButton2", 0)
            .set_times_limit("MedicineConfirm", 0)
            .set_times_limit("StoneConfirm", 0);

        return true;
    }
    return true;
}
