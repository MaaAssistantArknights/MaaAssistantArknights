#include "GeneralConfiger.h"

#include <meojson/json.h>

bool asst::GeneralConfiger::parse(const json::value& json)
{
    m_version = json.at("version").as_string();

    {
        const json::value& options_json = json.at("options");
        m_options.connect_type = static_cast<ConnectType>(options_json.at("connectType").as_integer());
        m_options.task_delay = options_json.at("taskDelay").as_integer();
        m_options.control_delay_lower = options_json.at("controlDelayRange")[0].as_integer();
        m_options.control_delay_upper = options_json.at("controlDelayRange")[1].as_integer();
        //m_options.print_window = options_json.at("printWindow").as_boolean();
        m_options.adb_extra_swipe_dist = options_json.get("adbExtraSwipeDist", 100);
        m_options.adb_extra_swipe_duration = options_json.get("adbExtraSwipeDuration", -1);

        auto& penguin_report = options_json.at("penguinReport");
        m_options.penguin_report.enable = penguin_report.get("enable", true);
        m_options.penguin_report.cmd_format = penguin_report.get("cmdFormat", std::string());
        m_options.penguin_report.server = penguin_report.get("server", "CN");

        if (options_json.exist("aipOcr")) {
            auto& aip_ocr = options_json.at("aipOcr");
            m_options.aip_ocr.enable = aip_ocr.get("enable", false);
            m_options.aip_ocr.accurate = aip_ocr.get("accurate", false);
            m_options.aip_ocr.client_id = aip_ocr.get("clientId", std::string());
            m_options.aip_ocr.client_secret = aip_ocr.get("clientSerect", std::string());
        }
    }

    for (const auto& [name, emulator_json] : json.at("emulator").as_object()) {
        EmulatorInfo emulator_info;
        emulator_info.name = name;

        const json::object& handle_json = emulator_json.at("handle").as_object();
        emulator_info.handle.class_name = handle_json.at("class").as_string();
        emulator_info.handle.window_name = handle_json.at("window").as_string();

        const json::object& adb_json = emulator_json.at("adb").as_object();
        emulator_info.adb.path = adb_json.at("path").as_string();

        for (const json::value& address_json : adb_json.at("addresses").as_array()) {
            emulator_info.adb.addresses.emplace_back(address_json.as_string());
        }
        emulator_info.adb.devices = adb_json.at("devices").as_string();
        emulator_info.adb.address_regex = adb_json.at("addressRegex").as_string();
        emulator_info.adb.connect = adb_json.at("connect").as_string();
        emulator_info.adb.click = adb_json.at("click").as_string();
        emulator_info.adb.swipe = adb_json.at("swipe").as_string();
        emulator_info.adb.display = adb_json.at("display").as_string();
        emulator_info.adb.display_format = adb_json.at("displayFormat").as_string();
        emulator_info.adb.screencap = adb_json.at("screencap").as_string();
        emulator_info.adb.release = adb_json.at("release").as_string();
        //emulator_info.adb.pullscreen = adb_json.at("pullscreen").as_string();

        m_emulators_info.emplace(name, std::move(emulator_info));
    }

    return true;
}
