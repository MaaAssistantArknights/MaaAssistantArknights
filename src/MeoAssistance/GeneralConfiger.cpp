#include "GeneralConfiger.h"

#include <json.h>

bool asst::GeneralConfiger::parse(const json::value& json)
{
    m_version = json.at("version").as_string();

    const json::value& options_json = json.at("options");
    {
        m_options.connect_type = static_cast<ConnectType>(options_json.at("connectType").as_integer());
        m_options.connect_remote_address = options_json.at("connectRemoteAddress").as_string();
        m_options.task_delay = options_json.at("taskDelay").as_integer();
        //m_options.identify_cache = options_json.at("identifyCache").as_boolean();
        m_options.control_delay_lower = options_json.at("controlDelayRange")[0].as_integer();
        m_options.control_delay_upper = options_json.at("controlDelayRange")[1].as_integer();
        m_options.print_window = options_json.at("printWindow").as_boolean();
        m_options.upload_to_penguin = options_json.at("uploadToPenguin").as_boolean();
        m_options.penguin_api = options_json.at("penguinApi").as_string();
        m_options.penguin_server = options_json.get("penguinServer", "CN");
        m_options.ocr_thread_number = options_json.at("ocrThreadNumber").as_integer();
    }

    for (const auto& [name, emulator_json] : json.at("emulator").as_object()) {
        EmulatorInfo emulator_info;
        emulator_info.name = name;

        const json::object& handle_json = emulator_json.at("handle").as_object();
        emulator_info.handle.class_name = handle_json.at("class").as_string();
        emulator_info.handle.window_name = handle_json.at("window").as_string();

        const json::object& adb_json = emulator_json.at("adb").as_object();
        emulator_info.adb.path = adb_json.at("path").as_string();
        emulator_info.adb.connect = adb_json.at("connect").as_string();
        emulator_info.adb.click = adb_json.at("click").as_string();
        emulator_info.adb.swipe = adb_json.at("swipe").as_string();
        emulator_info.adb.display = adb_json.at("display").as_string();
        emulator_info.adb.display_regex = adb_json.at("displayRegex").as_string();
        emulator_info.adb.screencap = adb_json.at("screencap").as_string();
        //emulator_info.adb.pullscreen = adb_json.at("pullscreen").as_string();

        emulator_info.path = emulator_json.get("path", std::string());

        m_emulators_info.emplace(name, std::move(emulator_info));
    }

    return true;
}