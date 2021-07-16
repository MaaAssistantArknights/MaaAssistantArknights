#include "Configer.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <Windows.h>

#include "json.h"
#include "AsstDef.h"

using namespace asst;

std::string Configer::m_curDir;
std::string Configer::m_version;
Options Configer::m_options;
std::unordered_map<std::string, TaskInfo> Configer::m_tasks;
std::unordered_map<std::string, SimulatorInfo> Configer::m_handles;

bool Configer::reload()
{
	std::string filename = getResDir() + "config.json";
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
			task_info.threshold = task_json["threshold"].as_double();
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
			else {
				DebugTraceError("Task: %s 's type error: %s", name.c_str(), type.c_str());
				return false;
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
			simulator_info.width = simulator_json["width"].as_integer();
			simulator_info.height = simulator_json["height"].as_integer();
			simulator_info.xOffset = simulator_json["xOffset"].as_integer();
			simulator_info.yOffset = simulator_json["yOffset"].as_integer();

			m_handles.emplace(name, simulator_info);
		}
	}
	catch (json::exception& e) {
		DebugTraceError("Load config json error!: %s", e.what());
		return false;
	}

	return true;
}

std::string Configer::getCurDir()
{
	if (m_curDir.empty()) {
		char exepath_buff[_MAX_PATH] = { 0 };
		::GetModuleFileNameA(NULL, exepath_buff, _MAX_PATH);
		std::string exepath(exepath_buff);
		m_curDir = exepath.substr(0, exepath.find_last_of('\\') + 1);
	}
	return m_curDir;
}

std::string Configer::getResDir()
{
	return getCurDir() + "resource\\";
}