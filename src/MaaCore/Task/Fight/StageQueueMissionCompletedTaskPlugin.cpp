#include "StageQueueMissionCompletedTaskPlugin.h"

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "StageDropsTaskPlugin.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/StageDropsImageAnalyzer.h"
#include "Vision/RegionOCRer.h"

bool asst::StageQueueMissionCompletedTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    return details.get("details", "task", "") == "StageQueue@EndOfAction";
}

void asst::StageQueueMissionCompletedTaskPlugin::set_drop_stats(std::unordered_map<std::string, int> drop_stats)
{
    m_drop_stats = std::move(drop_stats);
}

std::unordered_map<std::string, int> asst::StageQueueMissionCompletedTaskPlugin::get_drop_stats()
{
    return m_drop_stats;
}

bool asst::StageQueueMissionCompletedTaskPlugin::_run()
{
    LogTraceFunction;

    mission_completed();
    return true;
}

/// <summary>
/// 识别关卡是否为三星完成
/// </summary>
void asst::StageQueueMissionCompletedTaskPlugin::mission_completed()
{
    LogTraceFunction;

    sleep(Task.get("PRTS")->post_delay);
    if (need_exit()) {
        return;
    }

    StageDropsImageAnalyzer analyzer(ctrler()->get_image());
    if (!analyzer.analyze()) {
        Log.error(__FUNCTION__, "StageDropsImage Analyze failed");
        return;
    }
    auto&& [code, difficulty] = analyzer.get_stage_key();

    std::string stage_code = std::move(code);
    ranges::transform(stage_code, stage_code.begin(), [](char ch) -> char { return static_cast<char>(::toupper(ch)); });

    Log.info(__FUNCTION__, "Stage Code:", stage_code, "Stars:", analyzer.get_stars());

    json::value battle_info = basic_info_with_what("StageQueueMissionCompleted");
    battle_info["details"]["stage_code"] = stage_code;
    battle_info["details"]["stars"] = analyzer.get_stars();
    callback(AsstMsg::SubTaskExtraInfo, battle_info);

    drop_info_callback(stage_code, difficulty, analyzer);
}
// 抄自StageDropsTaskPlugin
void asst::StageQueueMissionCompletedTaskPlugin::drop_info_callback(std::string stage_code,
                                                                StageDifficulty stage_difficulty,
                                                                StageDropsImageAnalyzer analyzer)
{
    LogTraceFunction;

    std::unordered_map<std::string, int> cur_drops_count;
    std::vector<json::value> drops_vec;
    for (const auto& drop : analyzer.get_drops()) {
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

    json::value info = basic_info_with_what("StageDrops");
    json::value& details = info["details"];
    details["stars"] = analyzer.get_stars();
    details["stats"] = json::array(std::move(stats_vec));
    details["drops"] = json::array(std::move(drops_vec));
    json::value& stage = details["stage"];
    stage["stageCode"] = stage_code;
    if (!stage_code.empty()) {
        stage["stageId"] = StageDrops.get_stage_info(stage_code, stage_difficulty).stage_id;
    }

    callback(AsstMsg::SubTaskExtraInfo, info);
    m_cur_info_json = std::move(details);

    if (m_enable_penguin) {
        upload_to_penguin(stage_code, analyzer.get_stars());
    }
}
bool asst::StageQueueMissionCompletedTaskPlugin::set_server(std::string server)
{
    m_server = std::move(server);
    return true;
}
bool asst::StageQueueMissionCompletedTaskPlugin::set_enable_penguin(bool enable)
{
    m_enable_penguin = enable;
    return true;
}
bool asst::StageQueueMissionCompletedTaskPlugin::set_penguin_id(std::string id)
{
    m_penguin_id = std::move(id);
    return true;
}
bool asst::StageQueueMissionCompletedTaskPlugin::set_enable_yituliu(bool enable)
{
    // 暂时没用上，其他地方加了这里也加一个
    m_enable_yituliu = enable;
    return true;
}
void asst::StageQueueMissionCompletedTaskPlugin::upload_to_penguin(std::string stage_code, int stars)
{
    LogTraceFunction;

    if (m_server != "CN" && m_server != "US" && m_server != "JP" && m_server != "KR") {
        return;
    }

    json::value cb_info = basic_info();
    cb_info["subtask"] = "ReportToPenguinStats";

    // Doc: https://developer.penguin-stats_vec.io/public-api/api-v2-instruction/report-api
    std::string stage_id = m_cur_info_json.get("stage", "stageId", std::string());
    if (stage_id.empty()) {
        cb_info["why"] = "UnknownStage";
        cb_info["details"] = json::object { { "stage_code", stage_code } };
        callback(AsstMsg::SubTaskError, cb_info);
        return;
    }
    if (stars != 3) {
        cb_info["why"] = "NotThreeStars";
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
        if (drop_type == "UNKNOWN_DROP") {
            cb_info["why"] = "UnknownDropType";
            callback(AsstMsg::SubTaskError, cb_info);
            return;
        }
        if (ranges::find(filter, drop_type) == filter.cend()) {
            continue;
        }
        if (drop.at("itemId").as_string().empty()) {
            cb_info["why"] = "UnknownDrops";
            callback(AsstMsg::SubTaskError, cb_info);
            return;
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

    if (!m_report_penguin_task_ptr) {
        m_report_penguin_task_ptr = std::make_shared<ReportDataTask>(report_penguin_callback, this);
    }

    m_report_penguin_task_ptr->set_report_type(ReportType::PenguinStats)
        .set_body(body.to_string())
        .set_extra_headers(extra_headers)
        .set_retry_times(5)
        .run();
}
void asst::StageQueueMissionCompletedTaskPlugin::report_penguin_callback(AsstMsg msg, const json::value& detail,
                                                                     AbstractTask* task_ptr)
{
    LogTraceFunction;

    auto p_this = dynamic_cast<StageQueueMissionCompletedTaskPlugin*>(task_ptr);
    if (!p_this) {
        return;
    }

    if (msg == AsstMsg::SubTaskExtraInfo && detail.get("what", std::string()) == "PenguinId") {
        std::string id = detail.get("details", "id", std::string());
        p_this->m_penguin_id = id;
    }

    p_this->callback(msg, detail);
}
