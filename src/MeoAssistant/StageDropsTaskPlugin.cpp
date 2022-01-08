#include "StageDropsTaskPlugin.h"

#include <thread>
#include <chrono>

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
        || details.get("class", std::string()) != "class asst::ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "EndOfAction") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::StageDropsTaskPlugin::run(AbstractTask* ptr)
{
    ProcessTask* cast_ptr = dynamic_cast<ProcessTask*>(ptr);

    set_startbutton_delay(cast_ptr);

    recognize_drops();
    if (need_exit()) {
        return false;
    }
    drop_info_callback();

    auto upload_future = std::async(
        std::launch::async,
        std::bind(&StageDropsTaskPlugin::upload_to_penguin, this));
    m_upload_pending.emplace_back(std::move(upload_future));

    return true;
}

void asst::StageDropsTaskPlugin::recognize_drops()
{
    sleep(Task.get("PRTS")->rear_delay);
    if (need_exit()) {
        return;
    }
    cv::Mat image = Ctrler.get_image(true);
    m_cur_drops = json::parse(Resrc.penguin().recognize(image)).value();
}

void asst::StageDropsTaskPlugin::drop_info_callback()
{
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

    json::value info = basic_info();
    info["what"] = "StageDrops";
    info["details"] = drops_details;

    callback(AsstMsg::SubTaskExtraInfo, info);
}

void asst::StageDropsTaskPlugin::set_startbutton_delay(ProcessTask* ptr)
{
    if (!m_startbutton_delay_setted) {
        int64_t start_times = Status.get("LastStartButton2");

        if (start_times > 0) {
            int64_t duration = time(nullptr) - start_times;
            int elapsed = Task.get("EndOfAction")->pre_delay + Task.get("PRTS")->rear_delay;
            int64_t delay = duration * 1000 - elapsed;
            ptr->set_rear_delay("StartButton2", static_cast<int>(delay));
        }
    }
}

void asst::StageDropsTaskPlugin::upload_to_penguin()
{
    auto& opt = Resrc.cfg().get_options();
    if (!opt.penguin_report.enable) {
        return;
    }

    // Doc: https://developer.penguin-stats.io/public-api/api-v2-instruction/report-api
    json::value body;
    body["server"] = opt.penguin_report.server;
    body["stageId"] = m_cur_drops["stage"]["stageId"];
    // To fix: https://github.com/MistEO/MeoAssistantArknights/issues/40
    body["drops"] = json::array();
    for (auto&& drop : m_cur_drops["drops"].as_array()) {
        if (!drop["itemId"].as_string().empty()) {
            body["drops"].as_array().emplace_back(drop);
        }
    }
    body["source"] = "MeoAssistant";
    body["version"] = Version;

    std::string body_escape = utils::string_replace_all(body.to_string(), "\"", "\\\"");
    std::string cmd_line = utils::string_replace_all(opt.penguin_report.cmd_format, "[body]", body_escape);
    cmd_line = utils::string_replace_all(cmd_line, "[extra]", opt.penguin_report.extra_param);

    Log.trace("request_penguin |", cmd_line);

    std::string response = utils::callcmd(cmd_line);

    Log.trace("response:\n", response);
}
