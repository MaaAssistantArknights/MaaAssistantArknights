#include "GeneralConfiger.h"

#include "Utils/Logger.hpp"
#include <meojson/json.hpp>

bool asst::GeneralConfiger::parse(const json::value& json)
{
    m_version = json.at("version").as_string();

    {
        const json::value& options_json = json.at("options");
        m_options.task_delay = options_json.at("taskDelay").as_integer();
        m_options.control_delay_lower = options_json.at("controlDelayRange")[0].as_integer();
        m_options.control_delay_upper = options_json.at("controlDelayRange")[1].as_integer();
        // m_options.print_window = options_json.at("printWindow").as_boolean();
        m_options.adb_extra_swipe_dist = options_json.get("adbExtraSwipeDist", 100);
        m_options.adb_extra_swipe_duration = options_json.get("adbExtraSwipeDuration", -1);
        m_options.penguin_report.cmd_format = options_json.get("penguinReport", "cmdFormat", std::string());
        m_options.yituliu_report.cmd_format = options_json.get("yituliuReport", "cmdFormat", std::string());
        m_options.depot_export_template.ark_planner =
            options_json.get("depotExportTemplate", "arkPlanner", std::string());
        m_options.ocr_with_rawdata = options_json.get("ocrWithRawData", true);
    }

    for (const auto& [client_type, intent_name] : json.at("intent").as_object()) {
        m_intent_name[client_type] = intent_name.as_string();
    }

    for (const auto& [name, cfg_json] : json.at("connection").as_object()) {
        AdbCfg adb;

        adb.devices = cfg_json.at("devices").as_string();
        adb.address_regex = cfg_json.at("addressRegex").as_string();
        adb.connect = cfg_json.at("connect").as_string();
        adb.display_id = cfg_json.get("displayId", std::string());
        adb.uuid = cfg_json.at("uuid").as_string();
        adb.click = cfg_json.at("click").as_string();
        adb.swipe = cfg_json.at("swipe").as_string();
        adb.display = cfg_json.at("display").as_string();
        adb.display_format = cfg_json.at("displayFormat").as_string();
        adb.screencap_raw_with_gzip = cfg_json.at("screencapRawWithGzip").as_string();
        adb.screencap_raw_by_nc = cfg_json.at("screencapRawByNC").as_string();
        adb.nc_address = cfg_json.at("ncAddress").as_string();
        adb.nc_port = static_cast<unsigned short>(cfg_json.at("ncPort").as_integer());
        adb.screencap_encode = cfg_json.at("screencapEncode").as_string();
        adb.release = cfg_json.at("release").as_string();
        adb.start = cfg_json.at("start").as_string();
        adb.stop = cfg_json.at("stop").as_string();
        adb.push_minitouch = cfg_json.at("pushMinitouch").as_string();
        adb.chmod_minitouch = cfg_json.at("chmodMinitouch").as_string();
        adb.call_minitouch = cfg_json.at("callMinitouch").as_string();

        m_adb_cfg[name] = std::move(adb);
    }

    return true;
}
