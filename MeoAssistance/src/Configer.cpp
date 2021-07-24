#include "Configer.h"

#include <fstream>
#include <sstream>
#include <algorithm>

#include "json.h"
#include "Logger.hpp"

using namespace asst;

Configer::Configer(const Configer& rhs)
	: m_version(rhs.m_version),
	m_options(rhs.m_options),
	m_tasks(rhs.m_tasks),
	m_handles(rhs.m_handles)
{
}

Configer::Configer(Configer&& rhs) noexcept
	: m_version(std::move(rhs.m_version)),
	m_options(std::move(rhs.m_options)),
	m_tasks(std::move(rhs.m_tasks)),
	m_handles(std::move(rhs.m_handles))
{
}

bool Configer::reload(const std::string& filename)
{
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

	Configer temp;
	try {
		temp.m_version = root["version"].as_string();

		auto options_obj = root["options"].as_object();
		temp.m_options.identify_delay = options_obj["identifyDelay"].as_integer();
		temp.m_options.identify_cache = options_obj["identifyCache"].as_boolean();
		temp.m_options.control_delay_lower = options_obj["controlDelayRange"][0].as_integer();
		temp.m_options.control_delay_upper = options_obj["controlDelayRange"][1].as_integer();
		temp.m_options.print_window = options_obj["printWindow"].as_boolean();
		temp.m_options.print_window_delay = options_obj["printWindowDelay"].as_integer();

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
			if (task_json.exist("cacheThreshold")) {
				task_info.cache_threshold = task_json["cacheThreshold"].as_double();
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
			else if (type == "printwindow") {
				task_info.type = TaskType::PrintWindow;
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

			temp.m_tasks.emplace(name, task_info);
		}

		auto handle_obj = root["handle"].as_object();
		for (auto&& [name, emulator_json] : handle_obj) {
			EmulatorInfo emulator_info;
			emulator_info.name = name;

			auto window_arr = emulator_json["window"].as_array();
			for (auto&& info : window_arr) {
				HandleInfo handle_info;
				handle_info.class_name = info["class"].as_string();
				handle_info.window_name = info["window"].as_string();
				emulator_info.window.emplace_back(handle_info);
			}
			auto view_arr = emulator_json["view"].as_array();
			for (auto&& info : view_arr) {
				HandleInfo handle_info;
				handle_info.class_name = info["class"].as_string();
				handle_info.window_name = info["window"].as_string();
				emulator_info.view.emplace_back(handle_info);
			}
			auto ctrl_arr = emulator_json["control"].as_array();
			for (auto&& info : ctrl_arr) {
				HandleInfo handle_info;
				handle_info.class_name = info["class"].as_string();
				handle_info.window_name = info["window"].as_string();
				emulator_info.control.emplace_back(handle_info);
			}
			if (emulator_json.exist("adbControl")) {
				emulator_info.is_adb = true;
				// meojson的bug，暂时没空修，先转个字符串
				emulator_info.adb.path = StringReplaceAll(emulator_json["adbControl"]["path"].as_string(), "\\\\", "\\");
				emulator_info.adb.connect = emulator_json["adbControl"]["connect"].as_string();
				emulator_info.adb.click = emulator_json["adbControl"]["click"].as_string();
			}
			emulator_info.width = emulator_json["width"].as_integer();
			emulator_info.height = emulator_json["height"].as_integer();
			emulator_info.x_offset = emulator_json["xOffset"].as_integer();
			emulator_info.y_offset = emulator_json["yOffset"].as_integer();

			temp.m_handles.emplace(name, emulator_info);
		}
	}
	catch (json::exception& e) {
		DebugTraceError("Load config json error!", e.what());
		return false;
	}

	*this = std::move(temp);

	return true;
}

bool Configer::set_param(const std::string& type, const std::string& param, const std::string& value)
{
	// 暂时只用到了这些，总的参数太多了，后面要用啥再加上
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

std::optional<std::string> Configer::get_param(const std::string& type, const std::string& param)
{
	// 暂时只用到了这些，总的参数太多了，后面要用啥再加上
	if (type == "task.execTimes" && m_tasks.find(param) != m_tasks.cend()) {
		return std::to_string(m_tasks.at(param).exec_times);
	}
	return std::nullopt;
}

void Configer::clear_exec_times()
{
	for (auto&& t : m_tasks) {
		t.second.exec_times = 0;
	}
}

Configer& asst::Configer::operator=(const Configer& rhs)
{
	m_version = rhs.m_version;
	m_options = rhs.m_options;
	m_tasks = rhs.m_tasks;
	m_handles = rhs.m_handles;

	return *this;
}

Configer& asst::Configer::operator=(Configer&& rhs) noexcept
{
	m_version = std::move(rhs.m_version);
	m_options = std::move(rhs.m_options);
	m_tasks = std::move(rhs.m_tasks);
	m_handles = std::move(rhs.m_handles);

	return *this;
}