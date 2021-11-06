﻿/*
    MeoAssistance (CoreLib) - A part of the MeoAssistance-Arknight project
    Copyright (C) 2021 MistEO and Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "GeneralConfiger.h"

#include <json.h>

bool asst::GeneralConfiger::parse(const json::value& json) {
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
        m_options.penguin_report = options_json.at("penguinReport").as_boolean();
        m_options.penguin_report_cmd_line = options_json.at("penguinReportCmdLine").as_string();
        m_options.penguin_report_server = options_json.get("penguinReportServer", "CN");

        m_options.ocr_gpu_index = options_json.get("ocrGpuIndex", -1);
        m_options.ocr_thread_number = options_json.at("ocrThreadNumber").as_integer();

        m_options.adb_extra_swipe_dist = options_json.get("adbExtraSwipeDist", 100);
        m_options.adb_extra_swipe_duration = options_json.get("adbExtraSwipeDuration", -1);
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