#include "Configer.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <Windows.h>

#include "json.h"
#include "AsstDef.h"
#include "AsstAux.h"
#include "Logger.hpp"

using namespace asst;

std::string Configer::m_curDir;
std::string Configer::m_version;
Options Configer::m_options;
std::unordered_map<std::string, TaskInfo> Configer::m_tasks;
std::unordered_map<std::string, SimulatorInfo> Configer::m_handles;

bool Configer::reload()
{
	std::string filename = GetResourceDir() + "config.json";
	std::ifstream ifs(filename, std::ios::in);
	if (!ifs.is_open()) {
		return false;
	}
	std::stringstream iss;
	iss << ifs.rdbuf();
	ifs.close();
	std::string content(iss.str());

	auto ret = json::parser::parse(content);
	if (!ret) {
		return false;
	}

	auto root = std::move(ret).value();

	try {
		m_version = root["version"].as_string();

		auto options_obj = root["options"].as_object();
		m_options.delayType = options_obj["delay"]["type"].as_string();
		m_options.delayFixedTime = options_obj["delay"]["fixedTime"].as_integer();
		m_options.cache = options_obj["cache"].as_boolean();

		auto tasks_obj = root["tasks"].as_object();
		for (auto&& [name, task_json] : tasks_obj) {
			TaskInfo task_info;
			task_info.filename = task_json["filename"].as_string();
			if (task_json.exist("threshold")) {
				task_info.threshold = task_json["threshold"].as_double();
			}
			else {
				task_info.threshold = DefaultThreshold;
			}
			if (task_json.exist("cache_threshold")) {
				task_info.cache_threshold = task_json["cache_threshold"].as_double();
			}
			else {
				task_info.cache_threshold = DefaultCacheThreshold;
			}
			std::string type = task_json["type"].as_string();
			std::transform(type.begin(), type.end(), type.begin(), std::tolower);
			if (type == "clickself") {
				task_info.type = TaskType::ClickSelf;
			}
			else if (type == "clickrand") {
				task_info.type = TaskType::ClickRand;
			}
			else if (type == "donothing" || type.empty()) {
				task_info.type = TaskType::DoNothing;
			}
			else if (type == "stop") {
				task_info.type = TaskType::Stop;
			}
			else if (type == "clickrect") {
				task_info.type = TaskType::ClickRect;
				auto area_json = task_json["specificArea"].as_array();
				task_info.specific_area = Rect(
					area_json[0].as_integer(),
					area_json[1].as_integer(),
					area_json[2].as_integer(),
					area_json[3].as_integer());
			}
			else {
				DebugTraceError("Task:", name, "error:", type);
				return false;
			}
			if (task_json.exist("maxTimes")) {
				task_info.max_times = task_json["maxTimes"].as_integer();
			}
			if (task_json.exist("exceededNext")) {
				auto next_arr = task_json["exceededNext"].as_array();
				for (auto&& name : next_arr) {
					task_info.exceeded_next.emplace_back(name.as_string());
				}
			}
			else {
				task_info.exceeded_next.emplace_back("Stop");
			}
			if (task_json.exist("preDelay")) {
				task_info.pre_delay = task_json["preDelay"].as_integer();
			}
			if (task_json.exist("rearDelay")) {
				task_info.rear_delay = task_json["rearDelay"].as_integer();
			}
			if (task_json.exist("reduceOtherTimes")) {
				auto reduce_arr = task_json["reduceOtherTimes"].as_array();
				for (auto&& reduce : reduce_arr) {
					task_info.reduce_other_times.emplace_back(reduce.as_string());
				}
			}


			auto next_arr = task_json["next"].as_array();
			for (auto&& name : next_arr) {
				task_info.next.emplace_back(name.as_string());
			}

			m_tasks.emplace(name, task_info);
		}

		auto handle_obj = root["handle"].as_object();
		for (auto&& [name, simulator_json] : handle_obj) {
			SimulatorInfo simulator_info;

			auto window_arr = simulator_json["window"].as_array();
			for (auto&& info : window_arr) {
				HandleInfo handle_info;
				handle_info.className = info["class"].as_string();
				handle_info.windowName = info["window"].as_string();
				simulator_info.window.emplace_back(handle_info);
			}
			auto view_arr = simulator_json["view"].as_array();
			for (auto&& info : view_arr) {
				HandleInfo handle_info;
				handle_info.className = info["class"].as_string();
				handle_info.windowName = info["window"].as_string();
				simulator_info.view.emplace_back(handle_info);
			}
			auto ctrl_arr = simulator_json["control"].as_array();
			for (auto&& info : ctrl_arr) {
				HandleInfo handle_info;
				handle_info.className = info["class"].as_string();
				handle_info.windowName = info["window"].as_string();
				simulator_info.control.emplace_back(handle_info);
			}
			if (simulator_json.exist("adbControl")) {
				simulator_info.is_adb = true;
				// meojson的bug，暂时没空修，先转个字符串
				simulator_info.adb.path = StringReplaceAll(simulator_json["adbControl"]["path"].as_string(), "\\\\", "\\");
				simulator_info.adb.connect = simulator_json["adbControl"]["connect"].as_string();
				simulator_info.adb.click = simulator_json["adbControl"]["click"].as_string();
			}
			simulator_info.width = simulator_json["width"].as_integer();
			simulator_info.height = simulator_json["height"].as_integer();
			simulator_info.x_offset = simulator_json["xOffset"].as_integer();
			simulator_info.y_offset = simulator_json["yOffset"].as_integer();

			m_handles.emplace(name, simulator_info);
		}
	}
	catch (json::exception& e) {
		DebugTraceError("Load config json error!", e.what());
		return false;
	}

	return true;
}

bool Configer::setParam(const std::string& type, const std::string& param, const std::string& value)
{
	if (type == "task.type") {
		if (m_tasks.find(param) == m_tasks.cend()) {
			return false;
		}
		auto& task_info = m_tasks[param];
		std::string type = value;
		std::transform(type.begin(), type.end(), type.begin(), std::tolower);
		if (type == "clickself") {
			task_info.type = TaskType::ClickSelf;
		}
		else if (type == "clickrand") {
			task_info.type = TaskType::ClickRand;
		}
		else if (type == "donothing" || type.empty()) {
			task_info.type = TaskType::DoNothing;
		}
		else if (type == "stop") {
			task_info.type = TaskType::Stop;
		}
		else if (type == "clickrect") {
			task_info.type = TaskType::ClickRect;
		}
		else {
			DebugTraceError("Task", param, "'s type error:", type);
			return false;
		}
	}
	else if (type == "task.maxTimes") {
		if (m_tasks.find(param) == m_tasks.cend()) {
			return false;
		}
		m_tasks[param].max_times = std::stoi(value);
	}
	return true;
}

std::optional<std::string> Configer::getParam(const std::string& type, const std::string& param)
{
	if (type == "task.execTimes" && m_tasks.find(param) != m_tasks.cend()) {
		return std::to_string(m_tasks.at(param).exec_times);
	}
	return std::nullopt;
}

void Configer::clearExecTimes()
{
	for (auto&& t : m_tasks) {
		t.second.exec_times = 0;
	}
}