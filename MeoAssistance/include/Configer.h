#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

#include "AsstDef.h"

namespace asst {
	struct HandleInfo {
		std::string className;
		std::string windowName;
	};
	struct AdbCmd {
		std::string path;
		std::string connect;
		std::string click;
	};
	struct SimulatorInfo {
		std::vector<HandleInfo> window;
		std::vector<HandleInfo> view;
		std::vector<HandleInfo> control;
		bool is_adb = false;
		AdbCmd adb;
		int width = 0;
		int height = 0;
		int x_offset = 0;
		int y_offset = 0;
	};

	enum class TaskType {
		ClickSelf,
		ClickRand,
		DoNothing,
		Stop,
		ClickRect
	};

	struct TaskInfo {
		std::string filename;
		double threshold = 0;
		double cache_threshold = 0;
		TaskType type;
		std::vector<std::string> next;
		int exec_times = 0;
		int max_times = INT_MAX;
		std::vector<std::string> exceeded_next;
		asst::Rect specific_area;
		int pre_delay = 0;
		int rear_delay = 0;
	};
	struct Options {
		std::string delayType;
		int delayFixedTime = 0;
		bool cache = false;
	};

	class Configer
	{
	public:
		~Configer() = default;

		static bool reload();

		static std::string m_version;
		static Options m_options;
		static std::unordered_map<std::string, TaskInfo> m_tasks;
		static std::unordered_map<std::string, SimulatorInfo> m_handles;

		static bool setParam(const std::string& type, const std::string& param, const std::string& value);
		static std::optional<std::string> getParam(const std::string& type, const std::string& param);

		static void clearExecTimes();

		constexpr static double DefaultThreshold = 0.9;
		constexpr static double DefaultCacheThreshold = 0.9;
	private:
		Configer() = default;

		static std::string m_curDir;
	};
}
