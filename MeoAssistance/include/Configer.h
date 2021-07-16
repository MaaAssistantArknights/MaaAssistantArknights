#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace asst {
	struct HandleInfo {
		std::string className;
		std::string windowName;
	};
	struct SimulatorInfo {
		std::vector<HandleInfo> window;
		std::vector<HandleInfo> view;
		std::vector<HandleInfo> control;
		int width = 0;
		int height = 0;
		int xOffset = 0;
		int yOffset = 0;
	};

	enum class TaskType {
		ClickSelf,
		ClickRand,
		DoNothing,
		Stop
	};

	struct TaskInfo {
		std::string filename;
		double threshold = 0;
		TaskType type;
		std::vector<std::string> next;
		unsigned int exec_times = 0;
		unsigned int max_times = UINT_MAX;
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
		static std::string getCurDir();
		static std::string getResDir();

		static std::string m_version;
		static Options m_options;
		static std::unordered_map<std::string, TaskInfo> m_tasks;
		static std::unordered_map<std::string, SimulatorInfo> m_handles;

		static bool setParam(const std::string& type, const std::string& param, const std::string& value);
	private:
		Configer() = default;

		static std::string m_curDir;
	};
}
