#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

#include "AsstDef.h"

namespace asst {

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
