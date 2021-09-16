#include "Configer.h"

#include <fstream>
#include <sstream>
#include <algorithm>

#include <json.h>

#include "Logger.hpp"

using namespace asst;

bool asst::Configer::parse(json::value&& json)
{
	json::value root = std::move(json);

	m_version = root["version"].as_string();

	json::value& options_json = root["options"];
	{
		m_options.task_delay = options_json["taskDelay"].as_integer();
		//m_options.identify_cache = options_json["identifyCache"].as_boolean();
		m_options.control_delay_lower = options_json["controlDelayRange"][0].as_integer();
		m_options.control_delay_upper = options_json["controlDelayRange"][1].as_integer();
		m_options.print_window = options_json["printWindow"].as_boolean();
		m_options.print_window_delay = options_json["printWindowDelay"].as_integer();
		m_options.ocr_thread_number = options_json["ocrThreadNumber"].as_integer();
	}
	DebugTrace("Options", options_json.to_string());

	json::value& infrast_options_json = root["infrastOptions"];
	{
		m_infrast_options.dorm_threshold = infrast_options_json.get("dormThreshold", 0.3);
	}
	DebugTrace("InfrastOptions", infrast_options_json.to_string());

	for (auto&& [name, emulator_json] : root["emulator"].as_object()) {
		EmulatorInfo emulator_info;
		emulator_info.name = name;

		emulator_info.handle.class_name = emulator_json["handle"]["class"].as_string();
		emulator_info.handle.window_name = emulator_json["handle"]["window"].as_string();

		emulator_info.adb.path = emulator_json["adb"]["path"].as_string();
		emulator_info.adb.connect = emulator_json["adb"]["connect"].as_string();
		emulator_info.adb.click = emulator_json["adb"]["click"].as_string();
		emulator_info.adb.swipe = emulator_json["adb"]["swipe"].as_string();
		emulator_info.adb.display = emulator_json["adb"]["display"].as_string();
		emulator_info.adb.display_regex = emulator_json["adb"]["displayRegex"].as_string();
		emulator_info.adb.screencap = emulator_json["adb"]["screencap"].as_string();
		//emulator_info.adb.pullscreen = emulator_json["adb"]["pullscreen"].as_string();

		m_handles.emplace(name, std::move(emulator_info));
	}

	for (json::value& rep : root["infrastOcrReplace"].as_array()) {
		m_infrast_ocr_replace.emplace(rep.as_array()[0].as_string(), rep.as_array()[1].as_string());
	}
	for (json::value& rep : root["recruitOcrReplace"].as_array()) {
		m_recruit_ocr_replace.emplace(rep.as_array()[0].as_string(), rep.as_array()[1].as_string());
	}

	return true;
}